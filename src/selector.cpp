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
#include <selection.h>
#include <graph.h>
#include <blt/gfx/raycast.h>

extern blt::gfx::batch_renderer_2d renderer_2d;
extern blt::gfx::matrix_state_manager global_matrices;

void selector_t::render(graph_t& graph, blt::i32 width, blt::i32 height)
{

}

void selector_t::process_mouse(graph_t& graph, blt::i32 width, blt::i32 height)
{
    const auto mouse_pos = blt::make_vec2(blt::gfx::calculateRay2D(width, height, global_matrices.getScale2D(), global_matrices.getView2D(),
                                                                   global_matrices.getOrtho()));
    
    bool mouse_pressed = blt::gfx::isMousePressed(0) && blt::gfx::mousePressedLastFrame();
    
    for (const auto& [index, node] : blt::enumerate(graph.nodes))
    {
        const auto pos = node.getPosition();
        const auto dist = pos - mouse_pos;
        const auto mag = dist.magnitude();
        
        if (mag < node.getRenderObj().scale && mouse_pressed)
        {
            set_drag_selection(static_cast<blt::i64>(index));
            if (primary_selection == drag_selection || secondary_selection == drag_selection)
            {
                edge = false;
                break;
            }
            if (blt::gfx::isKeyPressed(GLFW_KEY_LEFT_SHIFT))
                set_secondary_selection(drag_selection);
            else
                set_primary_selection(drag_selection);
            break;
        }
    }
    if (blt::gfx::mouseReleaseLastFrame())
        set_drag_selection(-1);
    if (drag_selection >= 0 && drag_selection < static_cast<blt::i64>(graph.nodes.size()) && blt::gfx::isMousePressed(0))
    {
        graph.nodes[drag_selection].getPositionRef() = mouse_pos;
    }
}

void selector_t::draw_gui(graph_t& graph, blt::i32 width, blt::i32 height)
{
    im::SetNextItemOpen(true, ImGuiCond_Once);
    if (im::CollapsingHeader("Selection Information"))
    {
        im::Text("Silly (%ld) | (%ld || %ld)", drag_selection, primary_selection, secondary_selection);
    }
}

void selector_t::set_secondary_selection(blt::i64 n)
{
    secondary_selection = n;
}

void selector_t::set_primary_selection(blt::i64 n)
{
    primary_selection = n;
}

void selector_t::set_drag_selection(blt::i64 n)
{
    drag_selection = n;
}
