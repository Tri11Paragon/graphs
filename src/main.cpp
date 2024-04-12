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

void init()
{
    using namespace blt::gfx;
    
    global_matrices.create_internals();
    resources.load_resources();
    renderer_2d.create();
}

void update(std::int32_t width, std::int32_t height)
{
    global_matrices.update_perspectives(width, height, 90, 0.1, 2000);
    
    camera.update();
    camera.update_view(global_matrices);
    global_matrices.update();
    
    renderer_2d.render();
}

int main(int argc, const char** argv)
{
    blt::gfx::init(blt::gfx::window_data{"My Sexy Window", init, update}.setSyncInterval(1));
    global_matrices.cleanup();
    resources.cleanup();
    renderer_2d.cleanup();
    blt::gfx::cleanup();
}
