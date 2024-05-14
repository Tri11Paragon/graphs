/*
 *  Copyright (C) 2024  Brett Terpstra
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <blt/gfx/window.h>
#include "blt/gfx/renderer/resource_manager.h"
#include "blt/gfx/renderer/batch_2d_renderer.h"
#include "blt/gfx/renderer/camera.h"
#include <blt/gfx/framebuffer.h>
#include <blt/gfx/raycast.h>
#include <imgui.h>
#include <memory>
#include <random>
#include <blt/std/ranges.h>
#include <blt/std/time.h>
#include <blt/math/log_util.h>
#include <blt/gfx/input.h>
#include <blt/parse/templating.h>
#include <graph_base.h>
#include <force_algorithms.h>
#include <blt/gfx/renderer/shaders/pp_screen.frag>

blt::gfx::matrix_state_manager global_matrices;
blt::gfx::resource_manager resources;
blt::gfx::batch_renderer_2d renderer_2d(resources, global_matrices);
blt::gfx::first_person_camera_2d camera;
blt::u64 lastTime;
double ft = 0;
double fps = 0;
int sub_ticks = 1;

namespace im = ImGui;

struct bounding_box
{
    int min_x = 0;
    int min_y = 0;
    int max_x = 0;
    int max_y = 0;
    
    bounding_box(const int min_x, const int min_y, const int max_x, const int max_y): min_x(min_x), min_y(min_y), max_x(max_x), max_y(max_y)
    {
    }
    
    bool is_screen = true;
};

class graph_t
{
    private:
        std::vector<node> nodes;
        blt::hashset_t<edge, edge_hash> edges;
        blt::hashmap_t<blt::u64, blt::hashset_t<blt::u64> > connected_nodes;
        bool sim = false;
        bool run_infinitely = true;
        float sim_speed = 1;
        float threshold = 0;
        float max_force_last = 1;
        int current_iterations = 0;
        int max_iterations = 5000;
        std::unique_ptr<force_equation> equation;
        static constexpr float POINT_SIZE = 35;
        
        blt::i32 current_node = -1;
        
        void create_random_graph(bounding_box bb, const blt::size_t min_nodes, const blt::size_t max_nodes, const blt::f64 connectivity,
                                 const blt::f64 scaling_connectivity, const blt::f64 distance_factor)
        {
            // don't allow points too close to the edges of the window.
            if (bb.is_screen)
            {
                bb.max_x -= POINT_SIZE;
                bb.max_y -= POINT_SIZE;
                bb.min_x += POINT_SIZE;
                bb.min_y += POINT_SIZE;
            }
            static std::random_device dev;
            static std::uniform_real_distribution chance(0.0, 1.0);
            std::uniform_int_distribution node_count_dist(min_nodes, max_nodes);
            std::uniform_real_distribution pos_x_dist(static_cast<blt::f32>(bb.min_x), static_cast<blt::f32>(bb.max_x));
            std::uniform_real_distribution pos_y_dist(static_cast<blt::f32>(bb.min_y), static_cast<blt::f32>(bb.max_y));
            
            const auto node_count = node_count_dist(dev);
            
            for (blt::size_t i = 0; i < node_count; i++)
            {
                float x, y;
                do
                {
                    bool can_break = true;
                    x = pos_x_dist(dev);
                    y = pos_y_dist(dev);
                    for (const auto& node : nodes)
                    {
                        const auto& rp = node.getRenderObj().pos;
                        const float dx = rp.x() - x;
                        const float dy = rp.y() - y;
                        
                        if (const float dist = std::sqrt(dx * dx + dy * dy); dist <= POINT_SIZE)
                        {
                            can_break = false;
                            break;
                        }
                    }
                    if (can_break)
                        break;
                } while (true);
                nodes.push_back(node({x, y, POINT_SIZE}));
            }
            
            for (const auto& [index1, node1] : blt::enumerate(nodes))
            {
                for (const auto& [index2, node2] : blt::enumerate(nodes))
                {
                    if (index1 == index2)
                        continue;
                    const auto diff = node2.getPosition() - node1.getPosition();
                    const auto diff_sq = (diff * diff);
                    const auto dist = distance_factor / static_cast<float>(std::sqrt(diff_sq.x() + diff_sq.y()));
                    double dexp;
                    if (dist == 0)
                        dexp = 0;
                    else
                        dexp = 1 / (std::exp(dist) - dist);
                    if (const auto rand = chance(dev); rand <= connectivity && rand >= dexp * scaling_connectivity)
                        connect(index1, index2);
                }
            }
            
            std::uniform_int_distribution node_select_dist(0ul, nodes.size() - 1);
            for (blt::size_t i = 0; i < nodes.size(); i++)
            {
                if (connected_nodes[i].size() <= 1)
                {
                    for (blt::size_t j = connected_nodes[i].size(); j < 2; j++)
                    {
                        blt::u64 select;
                        do
                        {
                            select = node_select_dist(dev);
                            if (select != i && !connected_nodes[i].contains(select))
                                break;
                        } while (true);
                        connect(i, select);
                    }
                }
            }
        }
    
    public:
        graph_t() = default;
        
        void make_new(const bounding_box& bb, const blt::size_t min_nodes, const blt::size_t max_nodes, const blt::f64 connectivity)
        {
            create_random_graph(bb, min_nodes, max_nodes, connectivity, 0, 25);
            use_Eades();
        }
        
        void reset(const bounding_box& bb, const blt::size_t min_nodes, const blt::size_t max_nodes, const blt::f64 connectivity,
                   const blt::f64 scaling_connectivity, const blt::f64 distance_factor)
        {
            sim = false;
            current_iterations = 0;
            max_force_last = 1.0;
            nodes.clear();
            edges.clear();
            connected_nodes.clear();
            create_random_graph(bb, min_nodes, max_nodes, connectivity, scaling_connectivity, distance_factor);
        }
        
        void connect(const blt::u64 n1, const blt::u64 n2)
        {
            edges.insert(edge{n1, n2});
            connected_nodes[n1].insert(n2);
            connected_nodes[n2].insert(n1);
        }
        
        [[nodiscard]] bool connected(blt::u64 e1, blt::u64 e2) const
        {
            return edges.contains({e1, e2});
        }
        
        void render(const double frame_time)
        {
            if (sim && (current_iterations < max_iterations || run_infinitely) && max_force_last > threshold)
            {
                for (int _ = 0; _ < sub_ticks; _++)
                {
                    // calculate new forces
                    for (const auto& v1 : blt::enumerate(nodes))
                    {
                        blt::vec2 attractive;
                        blt::vec2 repulsive;
                        for (const auto& v2 : blt::enumerate(nodes))
                        {
                            if (v1.first == v2.first)
                                continue;
                            if (connected(v1.first, v2.first))
                                attractive += equation->attr(v1, v2);
                            repulsive += equation->rep(v1, v2);
                        }
                        v1.second.getVelocityRef() = attractive + repulsive;
                    }
                    max_force_last = 0;
                    // update positions
                    for (auto& v : nodes)
                    {
                        const float sim_factor = static_cast<float>(frame_time * sim_speed) * 0.05f;
                        v.getPositionRef() += v.getVelocityRef() * equation->cooling_factor(current_iterations) * sim_factor;
                        max_force_last = std::max(max_force_last, v.getVelocityRef().magnitude());
                    }
                    current_iterations++;
                }
            }
            
            for (const auto& point : nodes)
                renderer_2d.drawPointInternal(blt::gfx::render_info_t::make_info("parker_point", point.getOutlineColor(), point.getOutlineScale()),
                                              point.getRenderObj(), 15.0f);
            for (const auto& edge : edges)
            {
                if (edge.getFirst() >= nodes.size() || edge.getSecond() >= nodes.size())
                {
                    BLT_WARN("Edge Error %ld %ld %ld", edge.getFirst(), edge.getSecond(), nodes.size());
                } else
                {
                    auto n1 = nodes[edge.getFirst()];
                    auto n2 = nodes[edge.getSecond()];
                    auto draw_info = blt::gfx::render_info_t::make_info(edge.getColor());
                    auto outline_info = blt::gfx::render_info_t::make_info(edge.getOutlineColor());
                    blt::gfx::line2d_t line{n1.getRenderObj().pos, n2.getRenderObj().pos, edge.getThickness() * edge.getOutlineScale()};
                    renderer_2d.drawLine(draw_info, 5.0f, n1.getRenderObj().pos, n2.getRenderObj().pos, edge.getThickness());
                    renderer_2d.drawLineInternal(outline_info, line, 2.0f);
                }
            }
        }
        
        void reset_mouse_drag()
        {
            current_node = -1;
        }
        
        void process_mouse_drag(const blt::i32 width, const blt::i32 height)
        {
            const auto mouse_pos = blt::make_vec2(blt::gfx::calculateRay2D(width, height, global_matrices.getScale2D(), global_matrices.getView2D(),
                                                                           global_matrices.getOrtho()));
            
            if (current_node < 0)
            {
                for (const auto& [index, node] : blt::enumerate(nodes))
                {
                    const auto pos = node.getPosition();
                    const auto dist = pos - mouse_pos;
                    
                    if (const auto mag = dist.magnitude(); mag < POINT_SIZE)
                    {
                        current_node = static_cast<blt::i32>(index);
                        break;
                    }
                }
            } else
                nodes[current_node].getPositionRef() = mouse_pos;
        }
        
        void use_Eades()
        {
            equation = std::make_unique<Eades_equation>();
        }
        
        void use_Fruchterman_Reingold()
        {
            equation = std::make_unique<Fruchterman_Reingold_equation>();
        }
        
        void start_sim()
        {
            sim = true;
        }
        
        void stop_sim()
        {
            sim = false;
        }
        
        [[nodiscard]] std::string getSimulatorName() const
        {
            return equation->name();
        }
        
        [[nodiscard]] auto* getSimulator() const
        {
            return equation.get();
        }
        
        [[nodiscard]] auto getCoolingFactor() const
        {
            return equation->cooling_factor(current_iterations);
        }
        
        void reset_iterations()
        {
            current_iterations = 0;
        }
        
        [[nodiscard]] bool& getIterControl()
        {
            return run_infinitely;
        }
        
        [[nodiscard]] float& getSimSpeed()
        {
            return sim_speed;
        }
        
        [[nodiscard]] float& getThreshold()
        {
            return threshold;
        }
        
        [[nodiscard]] int& getMaxIterations()
        {
            return max_iterations;
        }
        
        [[nodiscard]] int numberOfNodes() const
        {
            return static_cast<int>(nodes.size());
        }
};

class engine_t
{
    private:
        graph_t graph;
        
        void draw_gui(const blt::gfx::window_data& data, const double ft)
        {
            if (im::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                static int min_nodes = 5;
                static int max_nodes = 25;
                
                static bounding_box bb{0, 0, data.width, data.height};
                
                static float connectivity = 0.12;
                static float scaling_connectivity = 0.5;
                static float distance_factor = 100;
                
                //im::SetNextItemOpen(true, ImGuiCond_Once);
                im::Text("FPS: %lf Frame-time (ms): %lf Frame-time (S): %lf", fps, ft * 1000.0, ft);
                im::Text("Number of Nodes: %d", graph.numberOfNodes());
                im::SetNextItemOpen(true, ImGuiCond_Once);
                if (im::CollapsingHeader("Help"))
                {
                    im::Text("You can use W/A/S/D to move the camera around");
                    im::Text("Q/E can be used to zoom in/out the camera");
                }
                if (im::CollapsingHeader("Graph Generation Settings"))
                {
                    im::Checkbox("Screen Auto-Scale", &bb.is_screen);
                    if (im::CollapsingHeader("Spawning Area"))
                    {
                        bool result = false;
                        result |= im::InputInt("Min X", &bb.min_x, 5, 100);
                        result |= im::InputInt("Max X", &bb.max_x, 5, 100);
                        result |= im::InputInt("Min Y", &bb.min_y, 5, 100);
                        result |= im::InputInt("Max Y", &bb.max_y, 5, 100);
                        if (result)
                            bb.is_screen = false;
                    }
                    if (bb.is_screen)
                    {
                        bb.max_x = data.width;
                        bb.max_y = data.height;
                        bb.min_x = 0;
                        bb.min_y = 0;
                    }
                    im::SeparatorText("Node Settings");
                    im::InputInt("Min Nodes", &min_nodes);
                    im::InputInt("Max Nodes", &max_nodes);
                    im::SliderFloat("Connectivity", &connectivity, 0, 1);
                    im::SliderFloat("Scaling Connectivity", &scaling_connectivity, 0, 1);
                    im::InputFloat("Distance Factor", &distance_factor, 5, 100);
                    if (im::Button("Reset Graph"))
                    {
                        graph.reset(bb, min_nodes, max_nodes, connectivity, scaling_connectivity, distance_factor);
                    }
                }
                im::SetNextItemOpen(true, ImGuiCond_Once);
                if (im::CollapsingHeader("Simulation Settings"))
                {
                    im::InputInt("Max Iterations", &graph.getMaxIterations());
                    im::Checkbox("Run Infinitely", &graph.getIterControl());
                    im::InputInt("Sub-ticks Per Frame", &sub_ticks);
                    im::InputFloat("Threshold", &graph.getThreshold(), 0.01, 1);
                    graph.getSimulator()->draw_inputs_base();
                    graph.getSimulator()->draw_inputs();
                    im::Text("Current Cooling Factor: %f", graph.getCoolingFactor());
                    im::SliderFloat("Simulation Speed", &graph.getSimSpeed(), 0, 4);
                }
                im::SetNextItemOpen(true, ImGuiCond_Once);
                if (im::CollapsingHeader("System Controls"))
                {
                    if (im::Button("Start"))
                        graph.start_sim();
                    im::SameLine();
                    if (im::Button("Stop"))
                        graph.stop_sim();
                    if (im::Button("Reset Iterations"))
                        graph.reset_iterations();
                    im::Text("Select a system:");
                    const auto current_sim = graph.getSimulatorName();
                    const char* items[] = {"Eades", "Fruchterman & Reingold"};
                    static int item_current = 0;
                    ImGui::ListBox("##SillyBox", &item_current, items, 2, 2);
                    
                    if (strcmp(items[item_current], current_sim.c_str()) != 0)
                    {
                        switch (item_current)
                        {
                            case 0:
                                graph.use_Eades();
                                BLT_INFO("Using Eades");
                                break;
                            case 1:
                                graph.use_Fruchterman_Reingold();
                                BLT_INFO("Using Fruchterman & Reingold");
                                break;
                            default:
                                BLT_WARN("This is not a valid selection! How did we get here?");
                                break;
                        }
                    }
                }
                im::End();
            }
        }
    
    public:
        void init(const blt::gfx::window_data& data)
        {
            graph.make_new({0, 0, data.width, data.height}, 5, 25, 0.2);
        }
        
        void render(const blt::gfx::window_data& data, const double ft)
        {
            draw_gui(data, ft);
            
            auto& io = ImGui::GetIO();
            
            if (!io.WantCaptureMouse && blt::gfx::isMousePressed(0))
                graph.process_mouse_drag(data.width, data.height);
            else
                graph.reset_mouse_drag();
            
            
            graph.render(ft);
        }
};

engine_t engine;

void init(const blt::gfx::window_data& data)
{
    using namespace blt::gfx;
    resources.setPrefixDirectory("../");
    
    resources.enqueue("res/debian.png", "debian");
    resources.enqueue("res/parker.png", "parker");
    resources.enqueue("res/parkerpoint.png", "parker_point");
    resources.enqueue("res/parker cat ears.jpg", "parkercat");
    
    global_matrices.create_internals();
    resources.load_resources();
    renderer_2d.create();
    
    engine.init(data);
    
    lastTime = blt::system::nanoTime();
}

void update(const blt::gfx::window_data& data)
{
    global_matrices.update_perspectives(data.width, data.height, 90, 0.1, 2000);
    
    //im::ShowDemoWindow();
    
    engine.render(data, ft);
    
    camera.update();
    camera.update_view(global_matrices);
    global_matrices.update();
    
    renderer_2d.render(data.width, data.height);
    
    const auto currentTime = blt::system::nanoTime();
    const auto diff = currentTime - lastTime;
    lastTime = currentTime;
    ft = static_cast<double>(diff) / 1000000000.0;
    fps = 1 / ft;
}

int main(int, const char**)
{
    blt::gfx::init(blt::gfx::window_data{"Graphing Lovers United", init, update, 1440, 720}.setSyncInterval(1));
    global_matrices.cleanup();
    resources.cleanup();
    renderer_2d.cleanup();
    blt::gfx::cleanup();
    
    return 0;
}
