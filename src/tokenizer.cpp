/*
 *  <Short Description>
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
#include <tokenizer.h>
#include <cctype>

namespace proc
{
    const std::vector<token_t>& tokenizer::tokenize()
    {
        while (has_next())
        {
            auto next = advance();
            if (std::isspace(next))
            {
                if (next == '\n')
                {
                    state = state_t::NEWLINE;
                    line_number++;
                }
                new_token();
                continue;
            }
            state_t determine = state_t::NONE;
            if (is_digit(next))
                determine = state_t::VALUE;
            else
            {
                switch (next)
                {
                    case '[':
                        determine = state_t::SQUARE_OPEN;
                        break;
                    case ']':
                        determine = state_t::SQUARE_CLOSE;
                        break;
                    default:
                        BLT_ERROR("Failed to parse data, error found at character index %ld on line %ld", current_pos, line_number);
                        BLT_ERROR("Context:");
                        BLT_ERROR(std::string_view(&data[std::max(0ul, current_pos - 40)], std::min(data.size() - current_pos, current_pos + 40)));
                        break;
                }
            }
            if (!can_state(determine))
            {
                begin = current_pos;
                new_token();
            }
        }
        return tokens;
    }
    
    bool tokenizer::is_digit(char c) const
    {
        return std::isdigit(c) || (state == state_t::VALUE && c == '.');
    }
}
