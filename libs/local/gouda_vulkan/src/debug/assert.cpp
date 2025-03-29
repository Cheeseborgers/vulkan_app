#include "debug/assert.hpp"

#ifndef NDEBUG
namespace assert_internal {

void assert_print(const std::source_location &location)
{
    std::cout << "Assertion failed at " << location.file_name() << ":" << location.line() << ", in function "
              << location.function_name() << "\n";
}

void assert_print(const std::source_location &location, std::string_view format, const std::format_args &args)
{
    std::cout << "Assertion failed at " << location.file_name() << "::" << location.line() << ", in function "
              << location.function_name() << ": " << std::vformat(format, args) << "\n";
}

void assert_impl(const std::source_location &location, bool check)
{
    if (!check) {
        assert_print(location);
        ASSERT_INTERNAL_DEBUGBREAK();
    }
}

} // namespace assert_internal
#endif