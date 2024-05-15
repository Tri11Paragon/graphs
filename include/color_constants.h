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

#ifndef GRAPHS_COLOR_CONSTANTS_H
#define GRAPHS_COLOR_CONSTANTS_H

#include <blt/math/vectors.h>

namespace color
{
    static inline constexpr blt::color4 POINT_OUTLINE_COLOR = blt::make_color(0, 0.6, 0.6);
    static inline constexpr blt::color4 POINT_HIGHLIGHT_COLOR = blt::make_color(0, 1.0, 1.0);
    static inline constexpr blt::color4 POINT_SELECT_COLOR = blt::make_color(0, 0.9, 0.7);
    
    static inline constexpr blt::color4 EDGE_COLOR = blt::make_color(0, 0.8, 0);
    static inline constexpr blt::color4 EDGE_OUTLINE_COLOR = blt::make_color(0, 1, 0);
}

#endif //GRAPHS_COLOR_CONSTANTS_H
