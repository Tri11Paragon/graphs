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

#ifndef GRAPHS_TOKENIZER_H
#define GRAPHS_TOKENIZER_H

#include <graph_base.h>
#include <vector>

namespace proc
{
    enum class token_e
    {
        SQUARE_OPEN,    // [
        SQUARE_CLOSE,   // ]
        CURLY_OPEN,     // {
        CURLY_CLOSE,    // }
        DOUBLE_QUOTE,   // "
        SINGLE_QUOTE,   // '
        SEMI,           // ;
        COLON,          // :
        COMMENT,        // # or //
        BLOCK_BEGIN,    // /* or /**
        BLOCK_CLOSE,    // */
        STAR,           // *
        TEXT,           // any text inside quotes
        IDENT,          // identifier
        VALUE,          // numeric value
        EQUAL,          // =
        COMMA,          // ,
        NEWLINE,        // \n
    };
    
    struct token_t
    {
        // the type of this token
        token_e token;
        // position inside file
        blt::size_t token_pos;
        // all data associated with token. will contain all text if text or the token characters otherwise
        std::string_view token_data;
    };
    
    /*
     * Example format:
     *
     * // this is a comment!
     * [[Textures]]
     * parker.png
     * silly.jpg; multiline.gif
     * boi.bmp
     *
     * # so is this!
     * [[Nodes]]
     * jim: texture=parker.png, location=[0.0f, 10.0f];
     * parker: texture=parker.png
     * brett
     *
     * // can't make the other kind but imagine it works
     * [[Edges]]
     * jim, parker; brett, parker
     * brett, jim
     *
     * [[Descriptions]]
     * brett: me silly
     * parker: boyfriend <3
     * jim: parker's friend
     */
    
    class tokenizer
    {
        public:
            explicit tokenizer(std::string_view str): data(str)
            {}
            
            explicit tokenizer(std::string&& str): data(std::move(str))
            {}
            
            const std::vector<token_t>& tokenize();
        
        private:
            std::string data;
            blt::size_t current_pos = 0;
            std::vector<token_t> tokens;
    };
}

#endif //GRAPHS_TOKENIZER_H
