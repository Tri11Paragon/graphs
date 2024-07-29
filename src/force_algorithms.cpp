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
#include <force_algorithms.h>
#include <blt/std/utility.h>

/**
 * Function for mixing between two points
 */
BLT_ATTRIB_NO_SIDE_EFFECTS inline float mix(const float v1, const float v2)
{
    return (v1 + v2) / 2.0f;
}

/**
 * --------------------------------------------------------
 *                      force_equation
 * --------------------------------------------------------
 */

force_equation::equation_data force_equation::calc_data(const std::pair<blt::size_t, node_t>& v1, const std::pair<blt::size_t, node_t>& v2)
{
    auto dir = dir_v(v1, v2);
    auto dir2 = dir_v(v2, v1);
    auto mag = dir.magnitude();
    auto mag2 = dir2.magnitude();
    auto unit = mag == 0 ? blt::vec2() : dir / mag;
    auto unit_inv = mag2 == 0 ? blt::vec2() : dir2 / mag2;
    auto mag_sq = mag * mag;
    return {unit, unit_inv, mag, mag_sq};
}

void force_equation::draw_inputs_base()
{
    namespace im = ImGui;
    im::SliderFloat("Initial Temperature", &initial_temperature, 1, 100);
    im::SliderFloat("Cooling Rate", &cooling_rate, 0, 0.999999, "%.6f");
    im::InputFloat("Min Cooling", &min_cooling, 0.5, 1);
}

/**
 * --------------------------------------------------------
 *                      Eades_equation
 * --------------------------------------------------------
 */

blt::vec2 Eades_equation::attr(const std::pair<blt::size_t, node_t>& v1, const std::pair<blt::size_t, node_t>& v2, const edge_t& edge) const
{
    auto data = calc_data(v1, v2);
    return (spring_constant * std::log(data.mag / edge.ideal_spring_length) * data.unit) - rep(v1, v2);
}

blt::vec2 Eades_equation::rep(const std::pair<blt::size_t, node_t>& v1, const std::pair<blt::size_t, node_t>& v2) const
{
    auto data = calc_data(v1, v2);
    // scaling factor included because of the scales this algorithm is working on (large viewport)
    auto scale = (mix(v1.second.repulsiveness, v2.second.repulsiveness) * 10000) / data.mag_sq;
    return scale * data.unit_inv;
}

void Eades_equation::draw_inputs()
{
    namespace im = ImGui;
    im::InputFloat("Spring Constant", &spring_constant, 0.25, 10);
}

/**
 * --------------------------------------------------------
 *              Fruchterman_Reingold_equation
 * --------------------------------------------------------
 */

blt::vec2 Fruchterman_Reingold_equation::attr(const std::pair<blt::size_t, node_t>& v1, const std::pair<blt::size_t, node_t>& v2, const edge_t& edge) const
{
    auto data = calc_data(v1, v2);
    float scale = data.mag_sq / edge.ideal_spring_length;
    return (scale * data.unit);
}

blt::vec2 Fruchterman_Reingold_equation::rep(const std::pair<blt::size_t, node_t>& v1, const std::pair<blt::size_t, node_t>& v2) const
{
    auto data = calc_data(v1, v2);
    const auto ideal_spring_length = conf::DEFAULT_SPRING_LENGTH;
    float scale = (ideal_spring_length * ideal_spring_length) / data.mag;
    return scale * data.unit_inv;
}