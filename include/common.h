#pragma once

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <source_location>
#include <stacktrace.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace sbrt
{
    enum class submodule
    {
        JIT,
        RAW_EXEC,
        OPT,
        ISEL,
        LOWER,
        MISC
    };

    class error : std::runtime_error
    {
        stacktrace::pointer_stacktrace trace;
        submodule module;
        std::source_location location;
        std::vector<std::string> notes;

    public:
        error(const std::string& message, submodule module, std::source_location location = std::source_location::current());

        constexpr auto add_note(std::string str) -> error&
        {
            notes.emplace_back(std::move(str));
            return *this;
        }

        [[nodiscard]] constexpr auto get_module() const -> submodule { return module; }
        [[nodiscard]] constexpr auto get_stacktrace() const -> const auto& { return trace; }
        [[nodiscard]] constexpr auto get_location() const -> const auto& { return location; }

        void print(std::ostream& out) const;
        [[noreturn]] inline void do_throw() { throw std::move(*this); }
    };

    template <typename... Ts>
    struct pack
    {
        inline static constexpr auto size = sizeof...(Ts);

        template <template <typename In> typename Mapper>
        using map = pack<Mapper<Ts>...>;

        template <typename... B>
        auto operator+(pack<B...> /*unused*/) -> pack<Ts..., B...>
        {
            return {};
        }
    };

    template <typename... T>
    using pack_append = decltype((pack{} + ... + T{}));

    template <size_t N>
    struct string_literal
    {
        constexpr string_literal(const char (&str)[N]) { std::copy_n(str, N, value); }

        char value[N]{};
    };

    template <typename... Ts>
    struct variant_cast_proxy
    {
        std::variant<Ts...> v;

        template <class... ToTs>
        operator std::variant<ToTs...>() const
        {
            return std::visit([](auto&& arg) -> std::variant<ToTs...> { return arg; }, v);
        }
    };

    template <typename... Ts>
    auto variant_cast(const std::variant<Ts...>& input) -> variant_cast_proxy<Ts...>
    {
        return {input};
    }

    template <typename... Ts>
    struct overload : Ts...
    {
        using Ts::operator()...;
    };

    template <class... Ts>
    overload(Ts...) -> overload<Ts...>;

} // namespace sbrt

#define sbrt_assert(submodule, e)                                                                                                                    \
    do                                                                                                                                               \
    {                                                                                                                                                \
        if (!(e))                                                                                                                                    \
        {                                                                                                                                            \
            ::sbrt::error("assertion '" #e "' failed", submodule).do_throw();                                                                        \
        }                                                                                                                                            \
    } while (0);

#define sbrt_unreachable(submodule)                                                                                                                  \
    do                                                                                                                                               \
    {                                                                                                                                                \
        ::sbrt::error("unreachable code reached", submodule).do_throw();                                                                             \
    } while (0);

#define sbrt_unreachable_m(submodule, msg)                                                                                                           \
    do                                                                                                                                               \
    {                                                                                                                                                \
        ::sbrt::error("unreachable code reached: " msg, submodule).do_throw();                                                                       \
    } while (0);


