/**
 * @file settings_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-11
 * @brief Application settings manager
 */

#include "core/settings_manager.hpp"

#include "logger.hpp"
#include "utility/filesystem.hpp"

// Definition of to_json and from_json for WindowSize
void to_json(nlohmann::json &json_data, const WindowSize &window_size)
{
    json_data = nlohmann::json{{"width", window_size.width}, {"height", window_size.height}};
}

void from_json(const nlohmann::json &json_data, WindowSize &window_size)
{
    json_data.at("width").get_to(window_size.width);
    json_data.at("height").get_to(window_size.height);
}

// Definition of to_json and from_json for ApplicationSettings
void to_json(nlohmann::json &json_data, const ApplicationSettings &settings)
{
    json_data = nlohmann::json{{"size", settings.size},
                               {"fullscreen", settings.fullscreen},
                               {"vsync", settings.vsync},
                               {"refresh_rate", settings.refresh_rate}};
}

void from_json(const nlohmann::json &json_data, ApplicationSettings &settings)
{
    // Handle the nested WindowSize structure manually
    if (json_data.contains("size") && json_data["size"].is_object()) {
        settings.size.width = json_data["size"].value("width", 800); // Default value if missing
        settings.size.height = json_data["size"].value("height", 800);
    }

    settings.fullscreen = json_data.value("fullscreen", false);
    settings.vsync = json_data.value("vsync", false);
    settings.refresh_rate = json_data.value("refresh_rate", 60);
}

SettingsManager::SettingsManager(const std::string &filepath, bool auto_save, bool auto_load)
    : m_settings{}, m_filepath{filepath}, m_auto_save{auto_save}
{
    if (auto_load) {
        Load();
    }
}

SettingsManager::~SettingsManager()
{
    if (m_auto_save) {
        Save(); // Save settings on destruction
    }
}

void SettingsManager::Load()
{
    // Check if the file exists and is not empty
    if (!gouda::fs::IsFileExists(m_filepath)) {
        APP_LOG_WARNING("Settings file '{}' does not exist, creating with default settings.", m_filepath);
        Save(); // Create the file and save default settings
        return;
    }

    if (gouda::fs::IsFileEmpty(m_filepath)) {
        APP_LOG_WARNING("Settings file '{}' does not exist, creating with default setting.", m_filepath);
        Save();
        return;
    }

    std::ifstream file(m_filepath);
    if (!file.is_open()) {
        APP_LOG_WARNING("Settings file '{}' does not exist, creating with default settings.", m_filepath);
        Save();
        return;
    }

    try {
        nlohmann::json json_data;
        file >> json_data;

        // Check if JSON is an object before attempting to deserialize
        if (!json_data.is_object()) {
            throw std::runtime_error("Expected JSON object but found array or other type.");
        }

        json_data.get_to(m_settings); // Deserialize to m_settings
    }
    catch (const std::exception &e) {
        APP_LOG_INFO("Failed to parse settings file '{}' reason: {}, using default setting.", m_filepath, e.what());
    }

    APP_LOG_DEBUG("Settings loaded from '{}'.", m_filepath);
}

void SettingsManager::Save() const
{
    std::ofstream file(m_filepath);
    if (!file.is_open()) {
        APP_LOG_ERROR("Could not open settings file '{}' for saving.", m_filepath);
        return;
    }

    try {
        nlohmann::json json_data = m_settings; // Should be a JSON object
        file << json_data.dump(4);             // Pretty print with indentation
    }
    catch (const std::exception &e) {
        APP_LOG_ERROR("Failed to save settings file '{}' reason: {}.", m_filepath, e.what());
    }

    APP_LOG_DEBUG("Settings saved to '{}", m_filepath);
}

ApplicationSettings SettingsManager::GetSettings() const { return m_settings; }

void SettingsManager::SetSettings(const ApplicationSettings &new_settings)
{
    if (new_settings.size.area() == 0) {
        APP_LOG_ERROR("New window size cannot have zero dimentions: {}x{}.", new_settings.size.width,
                      new_settings.height);
        return;
    }

    m_settings = new_settings;
    if (m_auto_save) {
        Save();
    }
}

void SettingsManager::SetWindowSize(WindowSize new_size)
{
    if (new_size.area() == 0) {
        APP_LOG_ERROR("New window size cannot have zero dimentions: {}x{}.", new_size.width, new_size.height);
        return;
    }

    m_settings.size = new_size;
    if (m_auto_save) {
        Save();
        ENGINE_LOG_DEBUG("Saved new window size {}x{}", new_size.width, new_size.height);
    }
}

void SettingsManager::SetFullScreen(bool enabled)
{
    m_settings.fullscreen = enabled;
    if (m_auto_save) {
        Save();
    }
}
void SettingsManager::SetVsync(bool enabled)
{
    m_settings.vsync = enabled;
    if (m_auto_save) {
        Save();
    }
}

void SettingsManager::SetRefreshRate(u16 rate)
{
    if (rate == 0) {
        APP_LOG_ERROR("New refresh rate (fps) cannot be zero.");
        return;
    }

    m_settings.refresh_rate = rate;
    if (m_auto_save) {
        Save();
    }
}
