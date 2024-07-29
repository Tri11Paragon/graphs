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

#ifndef GRAPHS_SELECTION_H
#define GRAPHS_SELECTION_H

#include <config.h>

class selector_t
{
    public:
        // called inside the info panel block, used for adding more information
        void draw_gui(graph_t& graph, blt::i32 width, blt::i32 height);
        
        // called once per frame, for rendering animations
        void render(graph_t& graph, blt::i32 width, blt::i32 height);
        
        // called once per frame assuming imgui doesn't want the mouse
        void process_mouse(graph_t& graph, blt::i32 width, blt::i32 height);
    private:
        void set_primary_selection(blt::i64 n);
        void set_drag_selection(blt::i64 n);
        void set_secondary_selection(blt::i64 n);
    private:
        blt::i64 drag_selection = -1;
        blt::i64 primary_selection = -1;
        blt::i64 secondary_selection = -1;
        bool edge = false;
};

#endif //GRAPHS_SELECTION_H
