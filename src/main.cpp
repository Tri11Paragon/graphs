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
#include <graph.h>
#include <blt/gfx/renderer/shaders/pp_screen.frag>

blt::gfx::matrix_state_manager global_matrices;
blt::gfx::resource_manager resources;
blt::gfx::batch_renderer_2d renderer_2d(resources, global_matrices);
blt::gfx::first_person_camera_2d camera;
blt::u64 lastTime;
double ft = 0;

namespace im = ImGui;

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
}

int main(int, const char**)
{
    blt::gfx::init(blt::gfx::window_data{"Force-Directed Graph Drawing", init, update, 1440, 720}.setSyncInterval(1));
    global_matrices.cleanup();
    resources.cleanup();
    renderer_2d.cleanup();
    blt::gfx::cleanup();
    
    return 0;
}
