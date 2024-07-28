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

#ifndef GRAPHS_LOADER_H
#define GRAPHS_LOADER_H

#include <config.h>
#include <graph_base.h>
#include <graph.h>
#include <optional>
#include <string_view>
#include <blt/gfx/window.h>

struct loader_t
{
    std::vector<std::pair<std::string, std::string>> textures;
    
    /**
     * if save path is present and a valid file it will be read from, otherwise path will be used as the default.
     */
    static std::optional<loader_t> load_for(engine_t& engine, const blt::gfx::window_data& data, std::string_view path,
                                            std::optional<std::string_view> save_path = {});
    
    /**
     * Saves engine to the path. Will also save any extra loader data like textures. Loader can be empty.
     */
    static void save_for(engine_t& engine, const loader_t& loader, std::string_view path);
};

#endif //GRAPHS_LOADER_H
