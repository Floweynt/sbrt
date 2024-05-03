#pragma once

#include "cast.h"
#include "common.h"
#include "dag.h"
#include "io.h"
#include "magic_enum.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <variant>

namespace sbrt::ir
{
    class ir_types
    {
    public:
        enum primitives
        {
            NONE,
            U8,
            U16,
            U32,
            U64,
            BOOL,
            P_U8,
            P_U16,
            P_U32,
            P_U64,
            P_BOOL,
            PTR,
            PRIMITIVE_MAX
        };

    private:
        std::uintptr_t data;

    public:
        constexpr ir_types(primitives primitive) : data(primitive) {}
        constexpr ir_types() : data(NONE) {}

        [[nodiscard]] constexpr auto is_primitive() const -> bool { return data < PRIMITIVE_MAX; }
        [[nodiscard]] constexpr auto primitive() const -> primitives { return static_cast<primitives>(data); }
        [[nodiscard]] constexpr auto type_desc() const -> void* { return as_vptr(data); }
        [[nodiscard]] constexpr auto is_ptr() const -> bool { return false; }
        auto read_uint(sbrt::byte_reader& reader) const -> uint64_t
        {
            switch (data)
            {
            case BOOL:
            case U8:
                return reader.read_u8();
            case U16:
                return reader.read_u16();
            case U32:
                return reader.read_u32();
            case U64:
                return reader.read_u64();
            default:
                throw std::runtime_error("illegal type");
            }
        }
    };

    enum class ir_opcode
    {
        NONE,
        IMM,
        ADD,
        SUB,
        MUL,
        UDIV,
        SDIV,
        MAX
    };

    using ir_imm_type = std::variant<uint64_t>;

    build_instruction_set(ir);

    struct ir_instr_specific_info
    {
        inline static constexpr auto get_operand_name(ir_dag_node* node, size_t index) -> std::string
        {
            switch (node->opcode)
            {
            case ir_opcode::MAX:
            case ir_opcode::NONE:
                sbrt_unreachable(submodule::MISC);
            case ir_opcode::IMM:
                return "";
            case ir_opcode::ADD:
            case ir_opcode::SUB:
            case ir_opcode::MUL:
            case ir_opcode::UDIV:
            case ir_opcode::SDIV:
                sbrt_assert(submodule::MISC, index < 2);
                return index == 0 ? "rhs" : "lhs";
            }
        }

        inline static constexpr auto get_imm_name(ir_dag_node* node, size_t index)
        {
            switch (node->opcode)
            {
            case ir_opcode::NONE:
            case ir_opcode::ADD:
            case ir_opcode::SUB:
            case ir_opcode::MUL:
            case ir_opcode::UDIV:
            case ir_opcode::SDIV:
            case ir_opcode::MAX:
                sbrt_unreachable(submodule::MISC);
            case ir_opcode::IMM:
                sbrt_assert(submodule::MISC, index == 0);
                return "value";
            }
        }

        inline static constexpr auto serialize_imm(const ir_imm_type& imm)
        {
            return std::visit(overload{[](uint64_t value) { return std::to_string(value); }}, imm);
        }

        inline static constexpr auto serialize_type(ir_types type) -> std::string
        {
            if (type.is_primitive())
            {
                return std::string(magic_enum::enum_name(type.primitive()));
            }

            sbrt_unreachable_m(submodule::MISC, "not implemented");
        }
    };
} // namespace sbrt::ir
