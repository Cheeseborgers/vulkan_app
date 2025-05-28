#pragma once
/**
 * @file core/settings_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-11
 * @brief Application settings manager
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */

#include "core/types.hpp"

#include <nlohmann/json.hpp>

struct ApplicationAudioSettings {
    f32 sound_volume;
    f32 music_volume;

    ApplicationAudioSettings() : sound_volume{0.5f}, music_volume{0.5f} {}
};

struct ApplicationSettings {
    WindowSize size;
    u16 refresh_rate;
    bool fullscreen;
    bool vsync;
    ApplicationAudioSettings audio_settings;

    ApplicationSettings() : size{800, 800}, refresh_rate{60}, fullscreen{false}, vsync{false} {}
};

void to_json(nlohmann::json &json_data, const WindowSize &window_size);
void from_json(const nlohmann::json &json_data, WindowSize &window_size);

void to_json(nlohmann::json &json_data, const ApplicationAudioSettings &audio_settings);
void from_json(const nlohmann::json &json_data, ApplicationAudioSettings &audio_settings);

void to_json(nlohmann::json &json_data, const ApplicationSettings &settings);
void from_json(const nlohmann::json &json_data, ApplicationSettings &settings);

class SettingsManager {
public:
    explicit SettingsManager(FilePath filepath, bool auto_save = false, bool auto_load = true);
    ~SettingsManager();

    void Load();
    void Save() const;

    [[nodiscard]] ApplicationSettings GetSettings() const;

    void SetAutoSave(const bool enabled) { m_auto_save = enabled; }
    void SetSettings(const ApplicationSettings &new_settings);
    void SetWindowSize(WindowSize new_size);
    void SetFullScreen(bool enabled);
    void SetVsync(bool enabled);
    void SetRefreshRate(u16 rate);

private:
    ApplicationSettings m_settings;
    FilePath m_filepath;
    bool m_auto_save;
    bool m_is_valid;
};
