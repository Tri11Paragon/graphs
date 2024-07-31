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

#ifndef GRAPHS_CONFIG_H
#define GRAPHS_CONFIG_H

#include <blt/std/types.h>
#include <blt/math/vectors.h>

namespace conf
{
    inline constexpr float DEFAULT_COOLING_FACTOR = 0.999999f;
    inline constexpr float DEFAULT_MIN_COOLING = 40;
    inline constexpr float DEFAULT_SPRING_LENGTH = 175.0;
    inline constexpr float DEFAULT_INITIAL_TEMPERATURE = 100;
    
    inline constexpr float POINT_SIZE = 75;
    inline constexpr float OUTLINE_SCALE = 1.25f;
    inline constexpr auto DEFAULT_IMAGE = "unknown";
    inline constexpr auto POINT_OUTLINE_COLOR = blt::make_color(0, 0.6, 0.6);
    inline constexpr auto POINT_HIGHLIGHT_COLOR = blt::make_color(0, 1.0, 1.0);
    inline constexpr auto POINT_SELECT_COLOR = blt::make_color(0, 0.9, 0.7);
    
    inline constexpr float DEFAULT_THICKNESS = 2.0f;
    inline constexpr auto EDGE_COLOR = blt::make_color(0, 0.8, 0);
    inline constexpr auto EDGE_OUTLINE_COLOR = blt::make_color(0, 1, 0);
}

class graph_t;
class selector_t;
struct loader_t;

#endif //GRAPHS_CONFIG_H
