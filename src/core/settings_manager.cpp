/**
 * @file core/settings_manager.cpp
 * @author GoudaCheeseburgers
 * @date 2025-03-11
 * @brief Application settings manager
 */
#include "core/settings_manager.hpp"

#include "debug/logger.hpp"
#include "utils/filesystem.hpp"

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

void to_json(nlohmann::json &json_data, const ApplicationAudioSettings &audio_settings)
{
    json_data =
        nlohmann::json{{"sound_volume", audio_settings.sound_volume}, {"music_volume", audio_settings.music_volume}};
}

void from_json(const nlohmann::json &json_data, ApplicationAudioSettings &audio_settings)
{
    json_data.at("sound_volume").get_to(audio_settings.sound_volume);
    json_data.at("music_volume").get_to(audio_settings.music_volume);
}

void to_json(nlohmann::json &json_data, const ApplicationSettings &settings)
{
    json_data = nlohmann::json{{"size", settings.size},
                               {"refresh_rate", settings.refresh_rate},
                               {"fullscreen", settings.fullscreen},
                               {"vsync", settings.vsync},
                               {"audio", settings.audio_settings}};
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

    // Handle the nested WindowSize structure manually
    if (json_data.contains("audio") && json_data["audio"].is_object()) {
        settings.audio_settings.sound_volume =
            json_data["audio"].value("sound_volume", 0.5f); // Default value if missing
        settings.audio_settings.music_volume = json_data["audio"].value("music_volume", 0.5f);
    }
}

SettingsManager::SettingsManager(FilePath filepath, bool auto_save, bool auto_load)
    : m_settings{}, m_filepath{filepath}, m_auto_save{auto_save}, m_is_valid{false}
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
    // Ensure the directory exists
    std::filesystem::path settings_dir = m_filepath.parent_path();
    std::error_code ec;
    if (!std::filesystem::exists(settings_dir, ec)) {
        if (!std::filesystem::create_directories(settings_dir, ec)) {
            APP_LOG_ERROR("Failed to create settings directory '{}'. Error: {}", settings_dir.string(), ec.message());
            return;
        }
    }

    // Check if the settings file exists
    if (!std::filesystem::exists(m_filepath, ec)) {
        APP_LOG_WARNING("Settings file '{}' does not exist, creating with default settings.", m_filepath.string());
        Save();
        return;
    }

    // Check if the settings file is empty
    if (std::filesystem::file_size(m_filepath, ec) == 0) {
        APP_LOG_WARNING("Settings file '{}' is empty, using default settings.", m_filepath.string());
        Save();
        return;
    }

    std::ifstream file(m_filepath);
    if (!file.is_open()) {
        APP_LOG_ERROR("Failed to open settings file '{}'.", m_filepath.string());
        return;
    }

    try {
        nlohmann::json json_data;
        file >> json_data;

        if (!json_data.is_object()) {
            throw std::runtime_error("Invalid JSON format (not an object).");
        }

        json_data.get_to(m_settings);
    }
    catch (const std::exception &e) {
        APP_LOG_WARNING("Failed to parse settings file '{}'. Error: {}, using default settings.", m_filepath.string(),
                        e.what());
        Save(); // Overwrite with default settings if parsing fails
    }

    APP_LOG_DEBUG("Settings loaded from '{}'.", m_filepath.string());
}

void SettingsManager::Save() const
{
    std::ofstream file(m_filepath);
    if (!file.is_open()) {
        APP_LOG_ERROR("Could not open settings file '{}' for saving.", m_filepath.string());
        return;
    }

    try {
        nlohmann::json json_data = m_settings; // Should be a JSON object
        file << json_data.dump(4);             // Pretty print with indentation
    }
    catch (const std::exception &e) {
        APP_LOG_ERROR("Failed to save settings file '{}' reason: {}.", m_filepath.string(), e.what());
    }

    APP_LOG_INFO("Settings saved to '{}", m_filepath.string());
}

ApplicationSettings SettingsManager::GetSettings() const { return m_settings; }

void SettingsManager::SetSettings(const ApplicationSettings &new_settings)
{
    if (new_settings.size.area() == 0) {
        APP_LOG_ERROR("New window size cannot have zero dimentions: {}x{}.", new_settings.size.width,
                      new_settings.size.height);
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
