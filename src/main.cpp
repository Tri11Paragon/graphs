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
#include <imgui.h>

blt::gfx::matrix_state_manager global_matrices;
blt::gfx::resource_manager resources;
blt::gfx::batch_renderer_2d renderer_2d(resources);
blt::gfx::first_person_camera camera;

void init(blt::gfx::window_context& context)
{
    using namespace blt::gfx;
    
    global_matrices.create_internals();
    resources.load_resources();
    renderer_2d.create();
    
}

float x = 50, y = 50;
float sx = 0.5, sy = 0.5;
float ax = 0.05, ay = 0.05;

void update(blt::gfx::window_context& context, std::int32_t width, std::int32_t height)
{
    global_matrices.update_perspectives(width, height, 90, 0.1, 2000);
    
    x += sx;
    y += sx;
    
    sx += ax;
    sy += ay;
    
    if (x > 256)
        sx *= -1;
    if (y > 256)
        sy *= -1;
    
    renderer_2d.drawLine(blt::vec4{1, 0, 1, 1}, 0.0f, blt::vec2{x,y}, blt::vec2{500, 500}, 5.0f);
    renderer_2d.drawLine(blt::vec4{1, 0, 0, 1}, 0.0f, blt::vec2{0,150}, blt::vec2{240, 0}, 12.0f);
    renderer_2d.drawPoint(blt::vec4{0, 1, 0, 1}, 1.0f, blt::vec2{500, 500}, 50.0f);
    renderer_2d.drawPoint(blt::vec4{0, 1, 1, 1}, 1.0f, blt::vec2{800, 500}, 256.0f);
    //renderer_2d.drawRectangle(blt::vec4{1,1,1,1}, -1.0f, blt::vec2{width / 2.0, height / 2.0}, blt::vec2{width, height});
    
    camera.update();
    camera.update_view(global_matrices);
    global_matrices.update();
    
    renderer_2d.render();
}

int main(int argc, const char** argv)
{
    blt::gfx::init(blt::gfx::window_data{"My Sexy Window", init, update, 1440, 720}.setSyncInterval(1));
    global_matrices.cleanup();
    resources.cleanup();
    renderer_2d.cleanup();
    blt::gfx::cleanup();
}
