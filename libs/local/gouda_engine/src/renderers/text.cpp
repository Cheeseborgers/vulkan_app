/**
 * @file renderers/text.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-08
 * @brief Engine msdf text module implementation
 */
#include "renderers/text.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "debug/logger.hpp"

namespace gouda {

Glyph::Glyph() : advance{0.0f}, plane_bounds{0.0f}, atlas_bounds{0.0f} {}

std::unordered_map<char, Glyph> load_msdf_glyphs(std::string_view json_path)
{
    std::ifstream file(json_path.data());
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + std::string(json_path));
    }

    nlohmann::json data;
    file >> data;

    if (!data.contains("glyphs")) {
        throw std::runtime_error("JSON file missing 'glyphs' field");
    }

    std::unordered_map<char, Glyph> glyphs;
    for (auto &[key, val] : data["glyphs"].items()) {
        Glyph glyph;
        if (!val.contains("advance") || !val.contains("planeBounds") || !val.contains("atlasBounds")) {
            continue; // Skip malformed glyphs
        }

        glyph.advance = val["advance"].get<float>();
        auto pb = val["planeBounds"];
        auto ab = val["atlasBounds"];
        glyph.plane_bounds = Rect<f32>{pb["left"].get<float>(), pb["bottom"].get<float>(), pb["right"].get<float>(),
                                       pb["top"].get<float>()};
        glyph.atlas_bounds = Rect<f32>{ab["left"].get<float>(), ab["bottom"].get<float>(), ab["right"].get<float>(),
                                       ab["top"].get<float>()};

        // Handle numeric or string keys
        char c = key.length() == 1 ? key[0] : static_cast<char>(std::stoi(key));
        glyphs[c] = glyph;
    }

    ENGINE_LOG_DEBUG("Loaded {} characters.", glyphs.size());
    return glyphs;
}

} // namespace gouda