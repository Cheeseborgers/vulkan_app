/**
 * @file text.cpp
 * @author GoudaCheeseburgers
 * @date 2025-04-08
 * @brief Engine msdf text module implementation
 */
#include "renderers/text.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "debug/logger.hpp"
#include "debug/throw.hpp"

namespace gouda {

// MSDFGlyph implementation  ----------------------------------------------
MSDFGlyph::MSDFGlyph() : advance{0.0f}, plane_bounds{0.0f}, atlas_bounds{0.0f} {}

// MSDFKerningPair implementation  ----------------------------------------------
MSDFKerningPair::MSDFKerningPair() : unicode1{0}, unicode2{0}, advance{0}, _padding1{0} {}

// MSDFAtlasParams implementation  ----------------------------------------------
MSDFAtlasParams::MSDFAtlasParams()
    : distance_range{0},
      distance_range_middle{0},
      font_size{0.0f},
      _padding0{0},
      atlas_size{0.0f},
      em_size{0.0f},
      line_height{0.0f},
      ascender{0.0f},
      descender{0.0f},
      underline_y{0.0f},
      underline_thickness{0.0f},
      y_origin_is_bottom{true},
      _padding1{},
      _padding2{0}
{
}

std::string MSDFAtlasParams::ToString() const
{
    return std::format("distance_range:{}, mid:{} f-size{}, atlas_size:{}, em_size:{}, line_height:{}, "
                       "ascender:{}, descender:{}, underline_y:{}, underline_thickness:{}, y_origin_is_bottom:{}",
                       distance_range, distance_range_middle, font_size, atlas_size.ToString(), em_size, line_height,
                       ascender, descender, underline_y, underline_thickness, y_origin_is_bottom);
}

// Functions --------------------------------------------------------------------
std::unordered_map<char, MSDFGlyph> load_msdf_glyphs(std::string_view json_path)
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

    MSDFGlyphMap glyph_map;
    for (auto &[key, val] : data["glyphs"].items()) {
        MSDFGlyph glyph;
        if (!val.contains("advance") || !val.contains("planeBounds") || !val.contains("atlasBounds")) {
            ENGINE_LOG_WARNING("Skipping malformed glyph for key: {}", key);
            continue;
        }

        glyph.advance = val["advance"].get<f32>();
        auto pb = val["planeBounds"];
        auto ab = val["atlasBounds"];
        glyph.plane_bounds =
            Rect<f32>{pb["left"].get<f32>(), pb["right"].get<f32>(), pb["bottom"].get<f32>(), pb["top"].get<f32>()};
        glyph.atlas_bounds =
            Rect<f32>{ab["left"].get<f32>(), ab["right"].get<f32>(), ab["bottom"].get<f32>(), ab["top"].get<f32>()};

        char c{0};
        try {
            c = key.length() == 1 ? key[0] : static_cast<char>(std::stoi(key));
        }
        catch (const std::exception &e) {
            ENGINE_LOG_WARNING("Invalid unicode key '{}': {}", key, e.what());
            continue;
        }

        glyph_map[c] = glyph; // Add the glyph for the map

        /*
        ENGINE_LOG_DEBUG(
            "Loaded glyph '{}'(unicode={}): advance={}, plane_bounds=({}, {}, {}, {}), atlas_bounds=({}, {}, {}, {})",
            c, static_cast<int>(c), glyph.advance, glyph.plane_bounds.left, glyph.plane_bounds.bottom,
            glyph.plane_bounds.right, glyph.plane_bounds.top, glyph.atlas_bounds.left, glyph.atlas_bounds.bottom,
            glyph.atlas_bounds.right, glyph.atlas_bounds.top);
            */
    }

    ENGINE_LOG_DEBUG("Loaded {} characters from {}", glyph_map.size(), json_path);
    return glyph_map;
}

MSDFAtlasParams load_msdf_atlas_params(StringView json_path)
{
    std::ifstream file(json_path.data());
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + String(json_path));
    }

    nlohmann::json data;
    file >> data;

    if (!data.contains("atlas") || !data.contains("metrics") || !data.contains("kerning")) {
        throw std::runtime_error("JSON file missing 'atlas', 'metrics' or 'kerning' field");
    }

    MSDFAtlasParams atlas_params{};
    const auto &atlas = data["atlas"];
    const auto &metrics = data["metrics"];

    // Atlas data
    atlas_params.distance_range = atlas["distanceRange"].get<f32>();
    atlas_params.distance_range_middle = atlas.value("distanceRangeMiddle", 0.0f);
    atlas_params.font_size = atlas["size"].get<f32>();
    atlas_params.atlas_size = {atlas["width"].get<f32>(), atlas["height"].get<f32>()};
    atlas_params.y_origin_is_bottom = atlas["yOrigin"].get<String>() == "bottom";

    // Metrics data
    atlas_params.em_size = metrics["emSize"].get<f32>();
    atlas_params.line_height = metrics["lineHeight"].get<f32>();
    atlas_params.ascender = metrics["ascender"].get<f32>();
    atlas_params.descender = metrics["descender"].get<f32>();
    atlas_params.underline_y = metrics["underlineY"].get<f32>();
    atlas_params.underline_thickness = metrics["underlineThickness"].get<f32>();

    // Kerning data
    if (data.contains("kerning")) {
        for (const auto &k : data["kerning"]) {
            MSDFKerningPair kp{};
            kp.unicode1 = k["unicode1"].get<u32>();
            kp.unicode2 = k["unicode2"].get<u32>();
            kp.advance = k["advance"].get<f32>();
            atlas_params.kerning.push_back(kp);
        }
    }

    return atlas_params;
}

} // namespace gouda