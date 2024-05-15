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

#ifndef GRAPHS_GRAPH_BASE_H
#define GRAPHS_GRAPH_BASE_H

#include <blt/gfx/renderer/batch_2d_renderer.h>
#include <blt/std/types.h>
#include <blt/std/assert.h>
#include <color_constants.h>


class node
{
    private:
        blt::gfx::point2d_t point;
        float outline_scale = 1.25f;
        blt::vec2 velocity;
        blt::color4 outline_color = color::POINT_OUTLINE_COLOR;
    public:
        explicit node(const blt::gfx::point2d_t& point): point(point)
        {}
        
        blt::vec2& getVelocityRef()
        {
            return velocity;
        }
        
        blt::vec2& getPositionRef()
        {
            return point.pos;
        }
        
        [[nodiscard]] const blt::vec2& getPosition() const
        {
            return point.pos;
        }
        
        [[nodiscard]] auto& getRenderObj() const
        {
            return point;
        }
        
        [[nodiscard]] float getOutlineScale() const
        {
            return outline_scale;
        }
        
        void setOutlineScale(float outlineScale)
        {
            outline_scale = outlineScale;
        }
        
        [[nodiscard]] const blt::color4& getOutlineColor() const
        {
            return outline_color;
        }
        
        void setOutlineColor(const blt::color4& c)
        {
            outline_color = c;
        }
};


class edge
{
    private:
        blt::u64 i1, i2;
        float outline_scale = 2.0f;
        float thickness = 2.0f;
        blt::color4 color = color::EDGE_COLOR;
        blt::color4 outline_color = color::EDGE_OUTLINE_COLOR;
    public:
        edge(blt::u64 i1, blt::u64 i2): i1(i1), i2(i2)
        {
            BLT_ASSERT(i1 != i2 && "Indices cannot be equal!");
        }
        
        inline friend bool operator==(edge e1, edge e2)
        {
            return (e1.i1 == e2.i1 || e1.i1 == e2.i2) && (e1.i2 == e2.i1 || e1.i2 == e2.i2);
        }
        
        [[nodiscard]] size_t getFirst() const
        {
            return i1;
        }
        
        [[nodiscard]] size_t getSecond() const
        {
            return i2;
        }
        
        [[nodiscard]] float getOutlineScale() const
        {
            return outline_scale;
        }
        
        void setOutlineScale(float outlineScale)
        {
            outline_scale = outlineScale;
        }
        
        [[nodiscard]] const blt::color4& getColor() const
        {
            return color;
        }
        
        void setColor(const blt::color4& c)
        {
            color = c;
        }
        
        [[nodiscard]] const blt::color4& getOutlineColor() const
        {
            return outline_color;
        }
        
        void setOutlineColor(const blt::color4& outlineColor)
        {
            outline_color = outlineColor;
        }
        
        [[nodiscard]] float getThickness() const
        {
            return thickness;
        }
        
        void setThickness(float t)
        {
            thickness = t;
        }
};

struct edge_hash
{
    blt::u64 operator()(const edge& e) const
    {
        return e.getFirst() * e.getSecond();
    }
};

#endif //GRAPHS_GRAPH_BASE_H
