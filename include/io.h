#pragma once

#include "cast.h"
#include <bit>
#include <cstdint>
#include <fstream>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace sbrt
{
    using u8_slice = std::span<uint8_t>;
    using u8_buf = std::vector<uint8_t>;

    class io_error : std::runtime_error
    {
    public:
        io_error(const std::string& str) : runtime_error(str) {}
    };

    namespace detail
    {
        class file_reader_impl
        {
            std::ifstream input;

        public:
            void open(const std::string& path) { input.open(path); }
            auto is_open() -> bool { return input.is_open(); }
            void close() { input.close(); }
            auto read(u8_slice buffer) -> size_t { return input.readsome(cast_ptr(buffer.data()), as_signed(buffer.size())); }
            auto skip(size_t size) { input.seekg(as_signed(size), std::ios_base::end); }
            auto off() const -> size_t { return input.gcount(); }
            auto has() -> bool { return !input.eof(); }
        };
    } // namespace detail

    template <typename T>
        requires std::is_unsigned_v<T>
    auto read_native_uint(detail::file_reader_impl& impl) -> T
    {
        if constexpr (sizeof(T) == 1)
        {
            uint8_t buf = 0;
            impl.read(u8_slice(&buf, 1));
            return buf;
        }
        else if constexpr (std::endian::native == std::endian::little)
        {
            T buf = 0;
            if (impl.read(u8_slice((uint8_t*)&buf, sizeof(buf))) != sizeof(buf))
            {
                throw io_error("error reading value: unexpected EOB");
            }
            return buf;
        }
        else
        {
            std::array<uint8_t, sizeof(T)> buf;
            if (impl.read(buf) != sizeof(T))
            {
                throw io_error("error reading value: unexpected EOB");
            }

            T result = 0;

            for (size_t i = 0; i < sizeof(T); i++)
            {
                result = result << 8;
                result |= buf[i];
            }

            return result;
        }
    }
    class byte_reader
    {
        detail::file_reader_impl impl;

        inline static constexpr uint8_t LEB_MASK = 0x7f;
        inline static constexpr uint8_t MAX_LEN = 63;
        inline static constexpr uint8_t CONT_MAX = 128;

    public:
#define MAKE_READ_U(n)                                                                                                                               \
    auto read_u##n() -> uint##n##_t { return read_native_uint<uint##n##_t>(impl); }
        MAKE_READ_U(8);
        MAKE_READ_U(16);
        MAKE_READ_U(32);
        MAKE_READ_U(64);
#undef MAKE_READ_U

        auto read_uleb() -> uint64_t
        {
            uint64_t res = 0;
            unsigned shift = 0;
            uint8_t curr = read_u8();
            do
            {
                if (!impl.has())
                {
                    throw io_error("failed to read uleb128: unexpected EOB");
                }

                uint64_t val = curr & LEB_MASK;

                if (shift >= MAX_LEN && ((shift == MAX_LEN && (val << shift >> shift) != val) || (shift > MAX_LEN && val != 0)))
                {
                    throw io_error("failed to read uleb128: too big");
                }

                res += val << shift;
                shift += 7;
                curr = read_u8();
            } while (curr >= CONT_MAX);

            return res;
        }

        auto read(size_t size) -> u8_buf
        {
            u8_buf buf(size);
            if (impl.read(buf) != size)
            {
                throw io_error("unexpected EOB");
            }
            return buf;
        }

        auto read(u8_slice buf)
        {
            if (impl.read(buf) != buf.size())
            {
                throw io_error("unexpected EOB");
            }
        }

        auto off() const -> size_t { return impl.off(); }

        void skip(size_t size) { impl.skip(size); }
    };
} // namespace sbrt
