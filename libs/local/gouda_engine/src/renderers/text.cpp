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

    if (!data.contains("glyphs") || !data.contains("atlas")) {
        throw std::runtime_error("JSON file missing 'glyphs' or 'atlas' field");
    }

    // Verify atlas size
    float atlas_width = data["atlas"]["width"].get<float>();
    float atlas_height = data["atlas"]["height"].get<float>();
    ENGINE_LOG_DEBUG("Loading font atlas: width={}, height={}", atlas_width, atlas_height);
    if (atlas_width != 312.0f || atlas_height != 312.0f) {
        ENGINE_LOG_WARNING("Atlas size mismatch: expected 312x312, got {}x{}", atlas_width, atlas_height);
    }

    std::unordered_map<char, Glyph> glyphs;
    for (auto &[key, val] : data["glyphs"].items()) {
        Glyph glyph;
        if (!val.contains("advance") || !val.contains("planeBounds") || !val.contains("atlasBounds")) {
            ENGINE_LOG_WARNING("Skipping malformed glyph for key: {}", key);
            continue;
        }

        glyph.advance = val["advance"].get<float>();
        auto pb = val["planeBounds"];
        auto ab = val["atlasBounds"];
        glyph.plane_bounds = Rect<f32>{pb["left"].get<float>(), pb["bottom"].get<float>(), pb["right"].get<float>(),
                                       pb["top"].get<float>()};
        glyph.atlas_bounds = Rect<f32>{ab["left"].get<float>(), ab["bottom"].get<float>(), ab["right"].get<float>(),
                                       ab["top"].get<float>()};

        char c;
        try {
            c = key.length() == 1 ? key[0] : static_cast<char>(std::stoi(key));
        }
        catch (const std::exception &e) {
            ENGINE_LOG_WARNING("Invalid unicode key '{}': {}", key, e.what());
            continue;
        }
        glyphs[c] = glyph;
        ENGINE_LOG_DEBUG(
            "Loaded glyph '{}'(unicode={}): advance={}, plane_bounds=({}, {}, {}, {}), atlas_bounds=({}, {}, {}, {})",
            c, static_cast<int>(c), glyph.advance, glyph.plane_bounds.left, glyph.plane_bounds.bottom,
            glyph.plane_bounds.right, glyph.plane_bounds.top, glyph.atlas_bounds.left, glyph.atlas_bounds.bottom,
            glyph.atlas_bounds.right, glyph.atlas_bounds.top);
    }

    ENGINE_LOG_DEBUG("Loaded {} characters from {}", glyphs.size(), json_path);
    return glyphs;
}

} // namespace gouda