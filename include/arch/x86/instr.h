#pragma once

#include "common.h"
#include "instr/dag.h"
#include <cstddef>
#include <cstdint>
#include <magic_enum.hpp>
#include <string>
#include <variant>

namespace sbrt::x86
{
    enum class mc_x86_types
    {
        NONE,
        U8,
        U16,
        U32,
        U64,
    };

    enum class mc_x86_opcode
    {
        NONE,
        ADD_ri,
        ADD_rr,
        SUB_ri,
        SUB_rr,
        MOV_ri,
        MAX
    };

    using mc_x86_imm_type = std::variant<uint64_t>;

    build_instruction_set(mc_x86);

    struct mc_x86_instr_specific_info
    {
        inline static constexpr auto get_operand_name(mc_x86_dag_node* node, size_t index) -> std::string
        {
            switch (node->opcode)
            {
            case mc_x86_opcode::SUB_ri:
            case mc_x86_opcode::ADD_ri:
                sbrt_assert(submodule::MISC, index == 0);
                return "operand";
            case mc_x86_opcode::SUB_rr:
            case mc_x86_opcode::ADD_rr:
                sbrt_assert(submodule::MISC, index < 2);
                return index == 0 ? "rhs" : "lhs";
                break;
            default:
                sbrt_unreachable(submodule::MISC);
            }
        }

        inline static constexpr auto get_imm_name(mc_x86_dag_node* node, size_t index)
        {
            switch (node->opcode)
            {
            case mc_x86_opcode::ADD_ri:
            case mc_x86_opcode::MOV_ri:
            case mc_x86_opcode::SUB_ri:
                sbrt_assert(submodule::MISC, index == 0);
                return "imm";
                break;
            default:
                sbrt_unreachable(submodule::MISC);
            }
        }

        inline static constexpr auto serialize_imm(const mc_x86_imm_type& imm)
        {
            return std::visit(overload{[](uint64_t value) { return std::to_string(value); }}, imm);
        }

        inline static constexpr auto serialize_type(mc_x86_types type) -> std::string { return std::string(magic_enum::enum_name(type)); }
    };
} // namespace sbrt::x86
