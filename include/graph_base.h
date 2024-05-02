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

class node
{
    private:
        blt::gfx::point2d_t point;
        blt::vec2 velocity;
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
};


class edge
{
    private:
        blt::u64 i1, i2;
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
};

struct edge_hash
{
    blt::u64 operator()(const edge& e) const
    {
        return e.getFirst() * e.getSecond();
    }
};

#endif //GRAPHS_GRAPH_BASE_H
