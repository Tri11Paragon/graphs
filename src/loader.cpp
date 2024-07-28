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
#include <loader.h>
#include <blt/std/logging.h>
#include <fstream>
#include <istream>
#include <random>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

template<typename T>
T load_with_default(const json& data, std::string_view key, T def)
{
    if (data.contains(key))
        return data[key].get<T>();
    return def;
}

std::optional<loader_t> loader_t::load_for(engine_t& engine, const blt::gfx::window_data& window_data, std::string_view path,
                                           std::optional<std::string_view> save_path)
{
    auto& graph = engine.graph;
    graph.clear();
    
    static std::random_device dev;
    std::uniform_real_distribution pos_x_dist(0.0, static_cast<blt::f64>(window_data.width));
    std::uniform_real_distribution pos_y_dist(0.0, static_cast<blt::f64>(window_data.height));
    
    if (save_path && std::filesystem::exists(*save_path))
        path = *save_path;
    if (!std::filesystem::exists(path))
    {
        BLT_WARN("Unable to load graph file!");
        return {};
    }
    std::ifstream file{std::string(path)};
    json data = json::parse(file);
    
    loader_t loader;
    if (data.contains("textures"))
    {
        for (const auto& texture : data["textures"])
        {
            auto texture_path = texture["path"].get<std::string>();
            auto texture_key = load_with_default(texture, "name", texture_path);
            loader.textures.emplace_back(texture_key, texture_path);
        }
    }
    
    if (data.contains("nodes"))
    {
        for (const auto& node : data["nodes"])
        {
            auto x = static_cast<blt::f32>(load_with_default(node, "x", pos_x_dist(dev)));
            auto y = static_cast<blt::f32>(load_with_default(node, "y", pos_y_dist(dev)));
            auto size = static_cast<blt::f32>(load_with_default(node, "size", static_cast<blt::f64>(conf::POINT_SIZE)));
            auto name = load_with_default<std::string>(node, "name", "unnamed");
            auto texture = load_with_default<std::string>(node, "texture", conf::DEFAULT_IMAGE);
            
            graph.names_to_node.insert({name, static_cast<blt::u64>(graph.nodes.size())});
            graph.nodes.emplace_back(blt::gfx::point2d_t{x, y, size});
            graph.nodes.back().name = std::move(name);
            graph.nodes.back().texture = std::move(texture);
        }
    }
    
    if (data.contains("connections"))
    {
        for (const auto& edge : data["edges"])
        {
            auto& nodes = edge["nodes"];
            auto index1 = nodes[0].get<std::string>();
            auto index2 = nodes[1].get<std::string>();
            auto ideal_length = load_with_default(edge, "length", conf::DEFAULT_SPRING_LENGTH);
            auto thickness = load_with_default(edge, "thickness", conf::DEFAULT_THICKNESS);
            
            ::edge e{graph.names_to_node[index1], graph.names_to_node[index2]};
            e.ideal_spring_length = ideal_length;
            e.thickness = thickness;
            
            graph.connect(e);
        }
    }
    
    return loader;
}

void loader_t::save_for(engine_t& engine, const loader_t& loader, std::string_view path)
{

}