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
            auto name = load_with_default<std::string>(node, "name", get_name());
            auto texture = load_with_default<std::string>(node, "texture", conf::DEFAULT_IMAGE);
            auto outline_scale = load_with_default(node, "scale", conf::OUTLINE_SCALE);
            auto outline_color = load_with_default(node, "color", json::array());
            
            BLT_ASSERT(!graph.names_to_node.contains(name) && "Graph node name must be unique!");
            graph.names_to_node.insert({name, static_cast<blt::u64>(graph.nodes.size())});
            graph.nodes.emplace_back(blt::gfx::point2d_t{x, y, size}, std::move(name));
            graph.nodes.back().texture = std::move(texture);
            graph.nodes.back().outline_scale = outline_scale;
            if (outline_color.is_array() && outline_color.size() >= 3)
            {
                auto r = static_cast<float>(outline_color[0].get<double>());
                auto g = static_cast<float>(outline_color[1].get<double>());
                auto b = static_cast<float>(outline_color[2].get<double>());
                if (outline_color > 3)
                    graph.nodes.back().outline_color = blt::vec4{r, g, b, static_cast<float>(outline_color[3].get<double>())};
                else
                    graph.nodes.back().outline_color = blt::make_color(r, g, b);
            }
        }
    }
    
    if (data.contains("edges"))
    {
        for (const auto& edge : data["edges"])
        {
            std::string index1;
            std::string index2;
            if (edge.is_array())
            {
                index1 = edge[0].get<std::string>();
                index2 = edge[1].get<std::string>();
            } else
            {
                auto& nodes = edge["nodes"];
                index1 = nodes[0].get<std::string>();
                index2 = nodes[1].get<std::string>();
            }
            auto ideal_length = load_with_default(edge, "length", conf::DEFAULT_SPRING_LENGTH);
            auto thickness = load_with_default(edge, "thickness", conf::DEFAULT_THICKNESS);
            
            edge_t e{graph.names_to_node[index1], graph.names_to_node[index2]};
            e.ideal_spring_length = ideal_length;
            e.thickness = thickness;
            
            graph.connect(e);
        }
    }
    
    if (data.contains("descriptions"))
    {
        for (const auto& desc : data["descriptions"])
        {
            if (auto node = graph.names_to_node.find(desc["name"].get<std::string>()); node != graph.names_to_node.end())
                graph.nodes[node->second].description = desc["description"];
            else
                BLT_WARN("Node %s doesn't exist!", desc["name"].get<std::string>().c_str());
        }
    }
    
    if (data.contains("relationships"))
    {
        for (const auto& desc : data["relationships"])
        {
            auto nodes = desc["nodes"];
            auto n1 = graph.names_to_node[nodes[0].get<std::string>()];
            auto n2 = graph.names_to_node[nodes[1].get<std::string>()];
            if (auto node = graph.edges.find({n1, n2}); node != graph.edges.end())
            {
                edge_t e = *node;
                e.description = desc["description"];
                graph.edges.erase({n1, n2});
                graph.edges.insert(e);
            } else
                BLT_WARN("Edge %s -> %s doesn't exist!", nodes[0].get<std::string>().c_str(), nodes[1].get<std::string>().c_str());
        }
    }
    
    return loader;
}

void loader_t::save_for(engine_t& engine, const loader_t& loader, std::string_view path)
{
    auto& graph = engine.graph;
    json data;
    data["textures"] = json::array();
    data["nodes"] = json::array();
    data["edges"] = json::array();
    data["descriptions"] = json::array();
    data["relationships"] = json::array();
    
    for (const auto& [key, t_path] : loader.textures)
    {
        data["textures"].push_back(json{
                {"name", key},
                {"path", t_path}
        });
    }
    
    for (const auto& node : graph.nodes)
    {
        const auto& color = node.outline_color;
        data["nodes"].push_back(json{
                {"name",    node.name},
                {"texture", node.texture},
                {"size",    node.getRenderObj().scale},
                {"x",       node.getPosition().x()},
                {"y",       node.getPosition().y()},
                {"scale",   node.outline_scale},
                {"color",   json::array({color.x(), color.y(), color.z(), color.w()})}
        });
        data["descriptions"].push_back(json{
                {"name", node.name},
                {"description", node.description}
        });
    }
    
    for (const auto& edge : graph.edges)
    {
        data["edges"].push_back(json{
                {"nodes", json::array_t{graph.nodes[edge.getFirst()].name, graph.nodes[edge.getSecond()].name}},
                {"length", edge.ideal_spring_length},
                {"thickness", edge.thickness}
        });
        data["relationships"].push_back(json{
                {"nodes", json::array_t{graph.nodes[edge.getFirst()].name, graph.nodes[edge.getSecond()].name}},
                {"description", edge.description}
        });
    }
    
    std::ofstream file{std::string(path)};
    file << data.dump(4);
}