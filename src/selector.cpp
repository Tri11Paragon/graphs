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
#include <blt/std/memory.h>

extern blt::gfx::batch_renderer_2d renderer_2d;
extern blt::gfx::matrix_state_manager global_matrices;

blt::expanding_buffer<char> name_input;
blt::expanding_buffer<char> texture_input;
blt::expanding_buffer<char> description_input;

struct callback_data_t
{
    std::string& str;
    blt::expanding_buffer<char>& buffer;
};

void from_string(std::string& str, blt::expanding_buffer<char>& buf)
{
    if (buf.size() <= str.size())
        buf.resize(str.size() + 1);
    std::memcpy(buf.data(), str.data(), str.size());
    buf.data()[str.size()] = '\0';
}

int text_input_callback(ImGuiInputTextCallbackData* data)
{
    auto [str, buffer] = *static_cast<callback_data_t*>(data->UserData);
    
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        buffer.resize(data->BufSize);
        data->Buf = buffer.data();
    } else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit || data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
        str = std::string_view{data->Buf, static_cast<blt::size_t>(data->BufTextLen)};
    
    return 0;
}

void selector_t::render(blt::i32 width, blt::i32 height)
{

}

void selector_t::process_mouse(blt::i32 width, blt::i32 height)
{
    const auto mouse_pos = blt::make_vec2(blt::gfx::calculateRay2D(width, height, global_matrices.getScale2D(), global_matrices.getView2D(),
                                                                   global_matrices.getOrtho()));
    
    bool mouse_pressed = blt::gfx::isMousePressed(0) && blt::gfx::mousePressedLastFrame();
    
    if (placement)
    {
        if (mouse_pressed)
        {
            placement = false;
            set_drag_selection(-1);
        } else if (blt::gfx::isKeyPressed(GLFW_KEY_ESCAPE))
        {
            destroy_node(primary_selection);
        }
    } else
    {
        for (const auto& [index, node] : blt::enumerate(graph.nodes))
        {
            const auto pos = node.getPosition();
            const auto dist = pos - mouse_pos;
            const auto mag = dist.magnitude();
            
            if (mag < node.getRenderObj().scale && mouse_pressed)
            {
                set_drag_selection(static_cast<blt::i64>(index));
                if (blt::gfx::isKeyPressed(GLFW_KEY_LEFT_SHIFT))
                    set_secondary_selection(drag_selection);
                else
                {
                    set_primary_selection(drag_selection);
                    set_secondary_selection(-1);
                }
                break;
            }
        }
        if (blt::gfx::mouseReleaseLastFrame())
            set_drag_selection(-1);
    }
    if (drag_selection >= 0 && drag_selection < static_cast<blt::i64>(graph.nodes.size()) && (blt::gfx::isMousePressed(0) || placement))
    {
        graph.nodes[drag_selection].getPositionRef() = mouse_pos;
    }
}

void selector_t::process_keyboard(blt::i32 width, blt::i32 height)
{
    const auto mouse_pos = blt::make_vec2(blt::gfx::calculateRay2D(width, height, global_matrices.getScale2D(), global_matrices.getView2D(),
                                                                   global_matrices.getOrtho()));
    if (blt::gfx::keyPressedLastFrame())
    {
        if (blt::gfx::isKeyPressed(GLFW_KEY_C))
        {
            if (placement)
                destroy_node(primary_selection);
            else
                create_placement_node(mouse_pos);
        } else if (blt::gfx::isKeyPressed(GLFW_KEY_X))
        {
            if (primary_selection != -1)
                destroy_node(primary_selection);
        }
    }
}

void selector_t::draw_gui(blt::i32 width, blt::i32 height)
{
    im::SetNextItemOpen(true, ImGuiCond_Once);
    if (im::CollapsingHeader("Selection Information"))
    {
        im::Text("Silly (%ld) | (%ld || %ld)", drag_selection, primary_selection, secondary_selection);
        if (primary_selection >= 0)
        {
            if (secondary_selection >= 0)
            {
            } else
            {
                im::InputFloat("Size", &graph.nodes[primary_selection].point.scale);
                callback_data_t name_data{graph.nodes[primary_selection].name, name_input};
                callback_data_t texture_data{graph.nodes[primary_selection].texture, texture_input};
                callback_data_t description_data{graph.nodes[primary_selection].description, description_input};
                im::InputText("Name", name_input.data(), name_input.size(),
                              ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackCompletion,
                              text_input_callback, &name_data);
                im::InputText("Texture", texture_input.data(), texture_input.size(),
                              ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackCompletion,
                              text_input_callback, &texture_data);
                im::InputTextMultiline("Description", description_input.data(), description_input.size(), {},
                                       ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackEdit | ImGuiInputTextFlags_CallbackCompletion,
                                       text_input_callback, &description_data);
            }
        }
    }
}

void selector_t::set_secondary_selection(blt::i64 n)
{
    secondary_selection = n;
    
}

void selector_t::set_primary_selection(blt::i64 n)
{
    primary_selection = n;
    if (primary_selection >= 0)
    {
        from_string(graph.nodes[primary_selection].name, name_input);
        from_string(graph.nodes[primary_selection].texture, texture_input);
        from_string(graph.nodes[primary_selection].description, description_input);
    }
}

void selector_t::set_drag_selection(blt::i64 n)
{
    drag_selection = n;
}

void selector_t::create_placement_node(const blt::vec2& pos)
{
    auto node = static_cast<blt::i64>(graph.nodes.size());
    graph.nodes.push_back(node_t{{pos, conf::POINT_SIZE}});
    set_drag_selection(node);
    set_primary_selection(node);
    placement = true;
}

void selector_t::destroy_node(blt::i64 node)
{
    auto& node_v = graph.nodes[node];
    graph.names_to_node.erase(node_v.name);
    erase_if(graph.edges, [node](const edge_t& v) {
        return static_cast<blt::i64>(v.getFirst()) == node || static_cast<blt::i64>(v.getSecond()) == node;
    });
    graph.connected_nodes.erase(node);
    for (auto& v : graph.connected_nodes)
    {
        v.second.erase(node);
    }
    
    graph.nodes.erase(graph.nodes.begin() + node);
    set_drag_selection(-1);
    set_primary_selection(-1);
    placement = false;
}
