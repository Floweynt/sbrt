#include "common.h"
#include <cstddef>
#include <fmt/core.h>
#include <magic_enum.hpp>
#include <ostream>
#include <source_location>
#include <stacktrace.h>
#include <stdexcept>
#include <string>

namespace sbrt
{
    error::error(const std::string& message, submodule module, std::source_location location)
        : std::runtime_error(message), trace(stacktrace::stacktrace()), module(module), location(location)
    {
    }

    void error::print(std::ostream& out) const
    {
        out << fmt::format("SBRT: Fatal Error: {}\n", what());
        out << fmt::format("In {}:{}:{} ({})\n", location.file_name(), location.line(), location.column(), location.function_name());
        out << fmt::format("In submodule {}\n", magic_enum::enum_name(module));
        out << fmt::format("Notes:\n");
        size_t index = 0;
        for (const auto& note : notes)
        {
            out << fmt::format("{}: {}\n", index, note);
        }

        out << fmt::format("\nStacktrace: \n");
        stacktrace::dump_stacktrace(stacktrace::get_symbols(trace), out);
    }

} // namespace sbrt
