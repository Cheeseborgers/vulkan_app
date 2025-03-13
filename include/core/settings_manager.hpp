#pragma once

/**
 * @file settings_manager.hpp
 * @author GoudaCheeseburgers
 * @date 2025-03-11
 * @brief Application settings manager
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "core/types.hpp"

#include <nlohmann/json.hpp>

struct ApplicationSettings {
    WindowSize size;
    bool fullscreen;
    bool vsync;
    u16 refresh_rate;

    ApplicationSettings() : size{800, 800}, fullscreen{false}, vsync{false}, refresh_rate{60} {}
};

void to_json(nlohmann::json &json_data, const WindowSize &window_size);
void from_json(const nlohmann::json &json_data, WindowSize &window_size);

void to_json(nlohmann::json &json_data, const ApplicationSettings &settings);
void from_json(const nlohmann::json &json_data, ApplicationSettings &settings);

class SettingsManager {
public:
    explicit SettingsManager(const std::string &filepath, bool auto_save = false, bool auto_load = true);
    ~SettingsManager();

    void Load();
    void Save() const;

    ApplicationSettings GetSettings() const;

    void SetAutoSave(bool enabled) { m_auto_save = enabled; }
    void SetSettings(const ApplicationSettings &new_settings);
    void SetWindowSize(WindowSize new_size);
    void SetFullScreen(bool enabled);
    void SetVsync(bool enabled);
    void SetRefreshRate(u16 rate);

private:
    ApplicationSettings m_settings;
    std::string m_filepath;
    bool m_auto_save;
};
