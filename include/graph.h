#pragma once
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

#ifndef GRAPHS_GRAPH_H
#define GRAPHS_GRAPH_H

#include <config.h>
#include <graph_base.h>
#include <force_algorithms.h>
#include <blt/gfx/window.h>
#include <blt/math/interpolation.h>
#include <blt/std/utility.h>

namespace im = ImGui;

enum class anim_state_t
{
    NONE,
    HIGHLIGHT_NODE,
    SELECT_NODE,
    HIGHLIGHT_TO_SELECT
};

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
        friend struct loader_t;
    private:
        std::vector<node> nodes;
        blt::hashmap_t<std::string, blt::u64> names_to_node;
        blt::hashset_t<edge, edge_hash> edges;
        blt::hashmap_t<blt::u64, blt::hashset_t<blt::u64>> connected_nodes;
        bool sim = false;
        bool run_infinitely = true;
        float sim_speed = 1;
        float threshold = 0;
        float max_force_last = 1;
        int current_iterations = 0;
        int max_iterations = 5000;
        std::unique_ptr<force_equation> equation;
        
        blt::i32 selected_node = -1;
        blt::quad_easing easing;
        blt::quint_easing highlight_easing;
        
        void create_random_graph(bounding_box bb, blt::size_t min_nodes, blt::size_t max_nodes, blt::f64 connectivity,
                                 blt::f64 scaling_connectivity, blt::f64 distance_factor);
    
    public:
        graph_t()
        {
            use_Eades();
        }
        
        void make_new(const bounding_box& bb, const blt::size_t min_nodes, const blt::size_t max_nodes, const blt::f64 connectivity)
        {
            create_random_graph(bb, min_nodes, max_nodes, connectivity, 0, 25);
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
        
        [[nodiscard]] std::optional<blt::ref<const edge>> connected(blt::u64 n1, blt::u64 n2) const
        {
            auto itr = edges.find(edge{n1, n2});
            if (itr == edges.end())
                return {};
            return *itr;
        }
        
        void render();
        
        void reset_mouse_drag()
        {
            if (selected_node != -1)
            {
                nodes[selected_node].outline_color = conf::POINT_OUTLINE_COLOR;
                easing.reset();
            }
            selected_node = -1;
        }
        
        void reset_mouse_highlight()
        {
            highlight_easing.reset();
        }
        
        void process_mouse_drag(blt::i32 width, blt::i32 height);
        
        void handle_mouse();
        
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
        friend struct loader_t;
    private:
        graph_t graph;
        
        void draw_gui(const blt::gfx::window_data& data);
    
    public:
        void init(const blt::gfx::window_data& data)
        {
            graph.make_new({0, 0, data.width, data.height}, 5, 25, 0.2);
        }
        
        void render(const blt::gfx::window_data& data)
        {
            draw_gui(data);
            
            auto& io = ImGui::GetIO();
            
            if (!io.WantCaptureMouse)
            {
                graph.process_mouse_drag(data.width, data.height);
            }
            
            graph.render();
        }
};

#endif //GRAPHS_GRAPH_H
