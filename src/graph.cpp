/*
 *  <Short Description>
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
#include <graph.h>
#include <random>
#include <blt/gfx/raycast.h>
#include <blt/std/ranges.h>

extern blt::gfx::batch_renderer_2d renderer_2d;
extern blt::gfx::matrix_state_manager global_matrices;

int sub_ticks = 1;

void graph_t::render(const double frame_time)
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
            auto draw_info = blt::gfx::render_info_t::make_info(edge.getColor(), edge.getOutlineColor(), edge.getOutlineScale());
            renderer_2d.drawLine(draw_info, 5.0f, n1.getRenderObj().pos, n2.getRenderObj().pos, edge.getThickness());
        }
    }
}

void graph_t::process_mouse_drag(const blt::i32 width, const blt::i32 height)
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

void graph_t::create_random_graph(bounding_box bb, const blt::size_t min_nodes, const blt::size_t max_nodes, const blt::f64 connectivity,
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

void engine_t::draw_gui(const blt::gfx::window_data& data, const double ft)
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
        im::Text("FPS: %lf Frame-time (ms): %lf Frame-time (S): %lf", 1.0 / ft, ft * 1000.0, ft);
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
