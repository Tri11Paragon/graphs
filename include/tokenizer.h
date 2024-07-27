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
    enum class state_t
    {
        NONE,           // Default state, no token found.
        SQUARE_OPEN,    // [
        SQUARE_CLOSE,   // ]
        CURLY_OPEN,     // {
        CURLY_CLOSE,    // }
        DOUBLE_QUOTE,   // "
        SINGLE_QUOTE,   // '
        SEMI,           // ;
        COLON,          // :
        COMMENT,        // # or //
        COMMENT_BEGIN,    // /* or /**
        COMMENT_CLOSE,    // */
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
        state_t token;
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
            [[nodiscard]] char peek(blt::size_t offset = 0) const
            {
                return data[current_pos + offset];
            }
            
            char advance()
            {
                return data[current_pos++];
            }
            
            bool has_next(blt::size_t size = 0)
            {
                return (current_pos + size) < data.size();
            }
            
            [[nodiscard]] bool is_digit(char c) const;
            
            void new_token()
            {
                if (state == state_t::NONE)
                    return;
                tokens.push_back({state, begin, {&data[begin], current_pos - begin}});
                state = state_t::NONE;
            }
            
            bool can_state(state_t s)
            {
                return s == state || state == state_t::NONE;
            }
        
        private:
            state_t state = state_t::NONE;
            blt::size_t current_pos = 0;
            blt::size_t line_number = 1;
            blt::size_t begin = current_pos;
            
            std::string data;
            
            std::vector<token_t> tokens;
    };
}

#endif //GRAPHS_TOKENIZER_H
