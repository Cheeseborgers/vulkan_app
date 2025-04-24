#include "debug/profiler.hpp"

#include <iomanip>

#include "debug/logger.hpp"
#include "utils/filesystem.hpp"

namespace gouda::internal::profiler {

#ifdef _WIN32
#include <windows.h>
inline int get_process_id() { return static_cast<int>(GetCurrentProcessId()); }
#else
#include <unistd.h>
inline int get_process_id() { return static_cast<int>(getpid()); }
#endif

Profiler::Profiler() : p_current_session(nullptr) {}

void Profiler::BeginSession(std::string_view name, std::string_view filepath)
{
    std::lock_guard lock{m_mutex};
    if (p_current_session) {
        ENGINE_LOG_ERROR("Tried creating session '{}' when session '{}' already open.", name, p_current_session->name);
        InternalEndSession();
    }

    // Ensure the profiling results directory exists.. if not create it.
    auto results_directory = FilePath(filepath).parent_path();
    auto results = fs::EnsureDirectoryExists(results_directory, true);
    if (!results) {
        ENGINE_LOG_ERROR("Error creating profiling directory: {}.", fs::error_to_string(results.error()));
        return;
    }

    m_output_stream.open(filepath.data());
    if (!m_output_stream.is_open()) {
        ENGINE_LOG_ERROR("Could not open profiler results file '{}'.", filepath);
        return;
    }

    p_current_session = std::make_unique<ProfilingSession>();
    p_current_session->name = name;
    WriteHeader();
}

void Profiler::EndSession()
{
    std::lock_guard lock{m_mutex};
    InternalEndSession();
}

void Profiler::Write(const ProfileResult &result)
{
    std::lock_guard lock{m_mutex};
    if (!p_current_session || !m_output_stream.is_open()) {
        return;
    }

    m_output_stream << ",{\"cat\":\"function\","
                    << "\"dur\":" << result.elapsed_time.count() << ',' << "\"name\":\"" << result.name << "\","
                    << "\"ph\":\"X\","
                    << "\"pid\":" << get_process_id() << ',' << "\"tid\":" << result.thread_id << ','
                    << "\"ts\":" << std::fixed << std::setprecision(3) << result.start.count() << "}";

    m_output_stream.flush();
}

void Profiler::WriteHeader()
{
    m_output_stream << "{\"other_data\": {},\"trace_events\":[{}";
    m_output_stream.flush();
}

void Profiler::WriteFooter()
{
    m_output_stream << "]}";
    m_output_stream.flush();
}

void Profiler::InternalEndSession()
{
    if (p_current_session) {
        WriteFooter();
        m_output_stream.close();
        p_current_session.reset();
    }
}

} // namespace gouda::internal::profiler