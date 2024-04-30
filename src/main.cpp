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
#include <imgui.h>
#include <random>
#include <blt/std/ranges.h>
#include <blt/std/assert.h>
#include <blt/std/time.h>
#include <blt/math/log_util.h>

blt::gfx::matrix_state_manager global_matrices;
blt::gfx::resource_manager resources;
blt::gfx::batch_renderer_2d renderer_2d(resources);
blt::gfx::first_person_camera_2d camera;
blt::gfx::fbo_t render_texture;
blt::u64 lastTime;
double ft = 0;
double fps = 0;
int sub_ticks = 1;

namespace im = ImGui;

class node
{
    private:
        blt::gfx::point2d_t point;
        blt::vec2 velocity;
    public:
        explicit node(const blt::gfx::point2d_t& point): point(point)
        {}
        
        blt::vec2& getVelocityRef()
        {
            return velocity;
        }
        
        blt::vec2& getPositionRef()
        {
            return point.pos;
        }
        
        [[nodiscard]] const blt::vec2& getPosition() const
        {
            return point.pos;
        }
        
        [[nodiscard]] auto& getRenderObj() const
        {
            return point;
        }
};


class edge
{
    private:
        blt::u64 i1, i2;
    public:
        edge(blt::u64 i1, blt::u64 i2): i1(i1), i2(i2)
        {
            BLT_ASSERT(i1 != i2 && "Indices cannot be equal!");
        }
        
        inline friend bool operator==(edge e1, edge e2)
        {
            return (e1.i1 == e2.i1 || e1.i1 == e2.i2) && (e1.i2 == e2.i1 || e1.i2 == e2.i2);
        }
        
        [[nodiscard]] size_t getFirst() const
        {
            return i1;
        }
        
        [[nodiscard]] size_t getSecond() const
        {
            return i2;
        }
};

struct edge_hash
{
    blt::u64 operator()(const edge& e) const
    {
        return e.getFirst() * e.getSecond();
    }
};

struct equation_variables
{
    float repulsive_constant = 12.0;
    float spring_constant = 24.0;
    float ideal_spring_length = 175.0;
    float initial_temperature = 69.5;
    float cooling_rate = 0.999;
};

class force_equation
{
    public:
        using node_pair = const std::pair<blt::size_t, node>&;
    protected:
        const equation_variables variables;
        
        struct equation_data
        {
            blt::vec2 unit;
            float mag, mag_sq;
            
            equation_data(blt::vec2 unit, float mag, float mag_sq): unit(unit), mag(mag), mag_sq(mag_sq)
            {}
        };
        
        inline static blt::vec2 dir_v(node_pair v1, node_pair v2)
        {
            return v2.second.getPosition() - v1.second.getPosition();
        }
        
        inline static equation_data calc_data(node_pair v1, node_pair v2)
        {
            auto dir = dir_v(v1, v2);
            auto mag = dir.magnitude();
            auto unit = mag == 0 ? blt::vec2() : dir / mag;
            auto mag_sq = mag * mag;
            return {unit, mag, mag_sq};
        }
    
    public:
        
        explicit force_equation(const equation_variables& variables): variables(variables)
        {}
        
        [[nodiscard]] virtual blt::vec2 attr(node_pair v1, node_pair v2) const = 0;
        
        [[nodiscard]] virtual blt::vec2 rep(node_pair v1, node_pair v2) const = 0;
        
        [[nodiscard]] virtual std::string name() const = 0;
        
        [[nodiscard]] virtual float cooling_factor(int t) const
        {
            return static_cast<float>(variables.initial_temperature * std::pow(variables.cooling_rate, t));
        }
        
        virtual ~force_equation() = default;
};

class Eades_equation : public force_equation
{
    public:
        explicit Eades_equation(const equation_variables& variables): force_equation(variables)
        {}
        
        [[nodiscard]] blt::vec2 attr(node_pair v1, node_pair v2) const final
        {
            auto data = calc_data(v1, v2);
            
            auto ideal = std::log(data.mag / variables.ideal_spring_length);
            
            return variables.spring_constant * ideal * data.unit;
        }
        
        [[nodiscard]] blt::vec2 rep(node_pair v1, node_pair v2) const final
        {
            auto data = calc_data(v1, v2);
            
            auto scale = variables.repulsive_constant / data.mag_sq;
            return scale * -data.unit;
        }
        
        [[nodiscard]] std::string name() const final
        {
            return "Eades";
        }
};

class Fruchterman_Reingold_equation : public force_equation
{
    public:
        explicit Fruchterman_Reingold_equation(const equation_variables& variables): force_equation(variables)
        {}
        
        [[nodiscard]] blt::vec2 attr(node_pair v1, node_pair v2) const final
        {
            auto data = calc_data(v1, v2);
            
            float scale = data.mag_sq / variables.ideal_spring_length;
            
            return scale * data.unit;
        }
        
        [[nodiscard]] blt::vec2 rep(node_pair v1, node_pair v2) const final
        {
            auto data = calc_data(v1, v2);
            
            float scale = (variables.ideal_spring_length * variables.ideal_spring_length) / data.mag;
            
            return scale * -data.unit;
        }
        
        [[nodiscard]] float cooling_factor(int t) const override
        {
            return force_equation::cooling_factor(t) * 0.025f;
        }
        
        [[nodiscard]] std::string name() const final
        {
            return "Fruchterman & Reingold";
        }
};

struct bounding_box
{
    int min_x = 0;
    int min_y = 0;
    int max_x = 0;
    int max_y = 0;
    
    bounding_box(int min_x, int min_y, int max_x, int max_y): min_x(min_x), min_y(min_y), max_x(max_x), max_y(max_y)
    {}
    
    bool is_screen = true;
};

class graph
{
    private:
        std::vector<node> nodes;
        blt::hashset_t<edge, edge_hash> edges;
        blt::hashmap_t<blt::u64, blt::hashset_t<blt::u64>> connected_nodes;
        bool sim = false;
        float sim_speed = 1;
        float threshold = 0.01;
        float max_force_last = 1;
        int current_iterations = 0;
        int max_iterations = 5000;
        equation_variables variables;
        std::unique_ptr<force_equation> equation;
        static constexpr float POINT_SIZE = 35;
        
        void create_random_graph(bounding_box bb, blt::size_t min_nodes, blt::size_t max_nodes, blt::f64 connectivity)
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
            
            auto node_count = node_count_dist(dev);
            
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
                        float dx = rp.x() - x;
                        float dy = rp.y() - y;
                        float dist = std::sqrt(dx * dx + dy * dy);
                        
                        if (dist <= POINT_SIZE)
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
            
            for (const auto& node1 : blt::enumerate(nodes))
            {
                for (const auto& node2 : blt::enumerate(nodes))
                {
                    if (node1.first == node2.first)
                        continue;
                    if (chance(dev) <= connectivity)
                        connect(node1.first, node2.first);
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
        graph() = default;
        
        graph(const bounding_box& bb, blt::size_t min_nodes, blt::size_t max_nodes, blt::f64 connectivity)
        {
            create_random_graph(bb, min_nodes, max_nodes, connectivity);
            use_Eades();
        }
        
        void reset(const bounding_box& bb, blt::size_t min_nodes, blt::size_t max_nodes, blt::f64 connectivity)
        {
            sim = false;
            current_iterations = 0;
            max_force_last = 1.0;
            nodes.clear();
            edges.clear();
            connected_nodes.clear();
            create_random_graph(bb, min_nodes, max_nodes, connectivity);
        }
        
        void connect(blt::u64 n1, blt::u64 n2)
        {
            edges.insert(edge{n1, n2});
            connected_nodes[n1].insert(n2);
            connected_nodes[n2].insert(n1);
        }
        
        [[nodiscard]] bool connected(blt::u64 e1, blt::u64 e2) const
        {
            return edges.contains({e1, e2});
        }
        
        void render(double frame_time)
        {
            if (sim && current_iterations < max_iterations && max_force_last > threshold)
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
                            attractive += equation->attr(v1, v2);
                            repulsive += equation->rep(v1, v2);
                        }
                        v1.second.getVelocityRef() = attractive + repulsive;
                    }
                    max_force_last = 0;
                    // update positions
                    for (auto& v : nodes)
                    {
                        float sim_factor = static_cast<float>(frame_time * sim_speed) * 0.05f;
                        v.getPositionRef() += v.getVelocityRef() * equation->cooling_factor(current_iterations) * sim_factor;
                        max_force_last = std::max(max_force_last, v.getVelocityRef().magnitude());
                    }
                    current_iterations++;
                }
            }
            
            for (const auto& point : nodes)
                renderer_2d.drawPointInternal("parker", point.getRenderObj(), 10.0f);
            for (const auto& edge : edges)
            {
                if (edge.getFirst() >= nodes.size() || edge.getSecond() >= nodes.size())
                {
                    BLT_WARN("Edge Error %ld %ld %ld", edge.getFirst(), edge.getSecond(), nodes.size());
                } else
                {
                    auto n1 = nodes[edge.getFirst()];
                    auto n2 = nodes[edge.getSecond()];
                    renderer_2d.drawLine(blt::make_color(0, 1, 0), 5.0f, n1.getRenderObj().pos, n2.getRenderObj().pos, 2.0f);
                }
            }
        }
        
        void use_Eades()
        {
            equation = std::make_unique<Eades_equation>(variables);
        }
        
        void use_Fruchterman_Reingold()
        {
            equation = std::make_unique<Fruchterman_Reingold_equation>(variables);
        }
        
        void start_sim()
        {
            sim = true;
        }
        
        void stop_sim()
        {
            sim = false;
        }
        
        std::string getSimulatorName()
        {
            return equation->name();
        }
        
        float& getSimSpeed()
        {
            return sim_speed;
        }
        
        float& getThreshold()
        {
            return threshold;
        }
        
        float& getSpringConstant()
        {
            return variables.spring_constant;
        }
        
        float& getInitialTemperature()
        {
            return variables.initial_temperature;
        }
        
        float& getCoolingRate()
        {
            return variables.cooling_rate;
        }
        
        float& getIdealSpringLength()
        {
            return variables.ideal_spring_length;
        }
        
        float& getRepulsionConstant()
        {
            return variables.repulsive_constant;
        }
        
        int& getMaxIterations()
        {
            return max_iterations;
        }
        
        [[nodiscard]] int numberOfNodes() const
        {
            return static_cast<int>(nodes.size());
        }
};

graph main_graph;

#ifdef __EMSCRIPTEN__
std::string resource_prefix = "../";
#else
std::string resource_prefix = "../";
#endif

void init(const blt::gfx::window_data& data)
{
    using namespace blt::gfx;
    resources.setPrefixDirectory(resource_prefix);
    
    resources.enqueue("res/debian.png", "debian");
    resources.enqueue("res/parker.png", "parker");
    resources.enqueue("res/parker cat ears.jpg", "parkercat");
    
    global_matrices.create_internals();
    resources.load_resources();
    renderer_2d.create();
    
    bounding_box bb(0, 0, data.width, data.height);
    main_graph = graph(bb, 5, 25, 0.2);
    lastTime = blt::system::nanoTime();
    
    //render_texture = fbo_t::make_multisample_render_texture(1440, 720, 4);
    //render_texture = fbo_t::make_multisample_render_target(1440, 720, 8);
    //render_texture = fbo_t::make_render_target(1440, 720);
    //render_texture = fbo_t::make_render_texture(1440, 720);
}

float x = 50, y = 50;
float sx = 0.5, sy = 0.5;
float ax = 0.05, ay = 0.05;

void update(const blt::gfx::window_data& data)
{
    global_matrices.update_perspectives(data.width, data.height, 90, 0.1, 2000);
    
    x += sx;
    y += sx;
    
    sx += ax;
    sy += ay;
    
    if (x > 256)
        sx *= -1;
    if (y > 256)
        sy *= -1;
    
    //im::ShowDemoWindow();
    if (im::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static int min_nodes = 5;
        static int max_nodes = 25;
        
        static bounding_box bb {0, 0, data.width, data.height};
        
        static float connectivity = 0.12;
        
        //im::SetNextItemOpen(true, ImGuiCond_Once);
        im::Text("FPS: %lf Frame-time (ms): %lf Frame-time (S): %lf", fps, ft * 1000.0, ft);
        im::Text("Number of Nodes: %d", main_graph.numberOfNodes());
        im::SetNextItemOpen(true, ImGuiCond_Once);
        if (im::CollapsingHeader("Help"))
        {
        
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
                {
                    bb.is_screen = false;
                }
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
            if (im::Button("Reset Graph"))
            {
                main_graph.reset(bb, min_nodes, max_nodes, connectivity);
            }
        }
        im::SetNextItemOpen(true, ImGuiCond_Once);
        if (im::CollapsingHeader("Simulation Settings"))
        {
            im::InputInt("Max Iterations", &main_graph.getMaxIterations());
            im::InputInt("Sub-ticks Per Frame", &sub_ticks);
            im::InputFloat("Threshold", &main_graph.getThreshold(), 0.01, 1);
            im::InputFloat("Repulsive Constant", &main_graph.getRepulsionConstant(), 0.25, 10);
            im::InputFloat("Spring Constant", &main_graph.getSpringConstant(), 0.25, 10);
            im::InputFloat("Ideal Spring Length", &main_graph.getIdealSpringLength(), 2.5, 10);
            im::SliderFloat("Initial Temperature", &main_graph.getInitialTemperature(), 1, 100);
            im::SliderFloat("Cooling Rate", &main_graph.getCoolingRate(), 0, 0.999999, "%.6f");
            im::SliderFloat("Simulation Speed", &main_graph.getSimSpeed(), 0, 4);
            if (im::Button("Start"))
                main_graph.start_sim();
            im::SameLine();
            if (im::Button("Stop"))
                main_graph.stop_sim();
        }
        im::SetNextItemOpen(true, ImGuiCond_Once);
        if (im::CollapsingHeader("System Controls"))
        {
            //im::Text("Current System: %s", main_graph.getSimulatorName().c_str());
            //im::
            auto current_sim = main_graph.getSimulatorName();
            const char* items[] = {"Eades", "Fruchterman & Reingold"};
            static int item_current = 0;
            ImGui::ListBox("##SillyBox", &item_current, items, 2, 2);
            
            if (strcmp(items[item_current], current_sim.c_str()) != 0)
            {
                switch (item_current)
                {
                    case 0:
                        main_graph.use_Eades();
                        BLT_INFO("Using Eades");
                        break;
                    case 1:
                        main_graph.use_Fruchterman_Reingold();
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
    
    //renderer_2d.drawLine(blt::vec4{1, 0, 1, 1}, 0.0f, blt::vec2{x, y}, blt::vec2{500, 500}, 5.0f);
    //renderer_2d.drawLine(blt::vec4{1, 0, 0, 1}, 0.0f, blt::vec2{0, 150}, blt::vec2{240, 0}, 12.0f);
    //renderer_2d.drawPoint(blt::vec4{0, 1, 0, 1}, 1.0f, blt::vec2{500, 500}, 50.0f);
    //renderer_2d.drawPoint("parkercat", 1.0f, blt::vec2{800, 500}, 256.0f);
    
    main_graph.render(ft);
    
    camera.update();
    camera.update_view(global_matrices);
    global_matrices.update();
    
    renderer_2d.render();
    
    auto currentTime = blt::system::nanoTime();
    auto diff = currentTime - lastTime;
    lastTime = currentTime;
    ft = static_cast<double>(diff) / 1000000000.0;
    fps = 1 / ft;
}

int main(int, const char**)
{
    blt::gfx::init(blt::gfx::window_data{"My Sexy Window", init, update, 1440, 720}.setSyncInterval(1));
    global_matrices.cleanup();
    resources.cleanup();
    renderer_2d.cleanup();
    blt::gfx::cleanup();
    
    return 0;
}
