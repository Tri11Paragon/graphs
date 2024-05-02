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

#ifndef GRAPHS_FORCE_ALGORITHMS_H
#define GRAPHS_FORCE_ALGORITHMS_H

#include <string>
#include <blt/std/types.h>
#include <blt/math/vectors.h>
#include <imgui.h>
#include <graph_base.h>

class force_equation
{
    public:
        using node_pair = const std::pair<blt::size_t, node>&;
    protected:
        float cooling_rate = 0.999999;
        float min_cooling = 40;
        float ideal_spring_length = 175.0;
        float initial_temperature = 100;
        
        struct equation_data
        {
            blt::vec2 unit, unit_inv;
            float mag, mag_sq;
            
            equation_data(blt::vec2 unit, blt::vec2 unit_inv, float mag, float mag_sq): unit(unit), unit_inv(unit_inv), mag(mag), mag_sq(mag_sq)
            {}
        };
        
        inline static blt::vec2 dir_v(node_pair v1, node_pair v2)
        {
            return v2.second.getPosition() - v1.second.getPosition();
        }
        
        static equation_data calc_data(node_pair v1, node_pair v2);
    
    public:
        
        [[nodiscard]] virtual blt::vec2 attr(node_pair v1, node_pair v2) const = 0;
        
        [[nodiscard]] virtual blt::vec2 rep(node_pair v1, node_pair v2) const = 0;
        
        [[nodiscard]] virtual std::string name() const = 0;
        
        [[nodiscard]] virtual float cooling_factor(int t) const
        {
            return std::max(static_cast<float>(initial_temperature * std::pow(cooling_rate, t)), min_cooling);
        }
        
        void draw_inputs_base();
        
        virtual void draw_inputs()
        {}
        
        virtual ~force_equation() = default;
};

class Eades_equation : public force_equation
{
    protected:
        float repulsive_constant = 24.0;
        float spring_constant = 12.0;
    public:
        [[nodiscard]] blt::vec2 attr(node_pair v1, node_pair v2) const final;
        
        [[nodiscard]] blt::vec2 rep(node_pair v1, node_pair v2) const final;
        
        [[nodiscard]] std::string name() const final
        {
            return "Eades";
        }
        
        void draw_inputs() override;
};

class Fruchterman_Reingold_equation : public force_equation
{
    public:
        [[nodiscard]] blt::vec2 attr(node_pair v1, node_pair v2) const final;
        
        [[nodiscard]] blt::vec2 rep(node_pair v1, node_pair v2) const final;
        
        [[nodiscard]] float cooling_factor(int t) const override
        {
            return force_equation::cooling_factor(t) * 0.025f;
        }
        
        [[nodiscard]] std::string name() const final
        {
            return "Fruchterman & Reingold";
        }
};

#endif //GRAPHS_FORCE_ALGORITHMS_H
