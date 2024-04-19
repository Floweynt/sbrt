#pragma once
#include "io.h"
#include <cstdint>
#include <variant>

// Bytecode format:
// bytecode_header:
// {
//      uint32_t magic;
//      uint16_t major;
//      uint16_t minor;
//      uleb128_64 constant_pool_length;
//      uleb128_64 function_pool_length;
//      uleb128_64 extra_table_length;
// }
// constant_pool:
// {
//      uint8_t flags = R | W | X | RESERVE_ONLY;
//      uleb128_64 length;
//      uint8_t buf[length];
// }[bytecode_header.constant_pool_entry]
// function_pool:
// {
//      uint64_t flags;
//      uleb128_64 length;
//      uint8_t data[];
// }[bytecode_header.function_pool_length]

namespace sbrt
{
    enum instr_code : uint16_t
    {
    };

    struct constant_table_entry
    {
        enum
        {
            PERM_R = 1 << 0,
            PERM_W = 1 << 1,
            PERM_X = 1 << 2,
            RESERVE_ONLY = 1 << 3
        };

        uint8_t flags{};
        size_t size{};
        u8_buf data;
    };

    struct function_table_entry
    {
        enum
        {
            ENTRYPOINT = 1 << 16,
            // TODO: add more flags
        };

        uint32_t flags{};
        u8_buf data;
    };

    template <typename T>
    struct wrap_native
    {
    };

    using type_def = std::variant<wrap_native<uint8_t>, wrap_native<uint16_t>, wrap_native<uint32_t>, wrap_native<uint64_t>>;

    class bytecode_file
    {
        uint16_t major = -1;
        uint16_t minor = -1;
        size_t entrypoint_index = -1ULL;
        std::vector<constant_table_entry> constant_table;
        std::vector<function_table_entry> function_table;

        inline static constexpr uint32_t MAGIC = 0xf00d1234;

    public:
        constexpr bytecode_file() = default;

        void read(byte_reader& reader);
        
        [[nodiscard]] constexpr auto get_ver_major() const { return major; }
        [[nodiscard]] constexpr auto get_ver_minor() const { return minor; }
    };
} // namespace sbrt
