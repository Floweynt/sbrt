#pragma once
#include "common.h"
#include "instr.h"
#include "instr/ir.h"
#include "isel/isel_builder.h"
#include "pass/isel_generic_pass.h"
#include <string>

namespace sbrt::x86
{
    using namespace sbrt::isel;

    // clang-format off
    using x86_target_type_lowering = target_type_lowering<
        mc_x86_types::NONE,
        mc_x86_types::U8,
        mc_x86_types::U16,
        mc_x86_types::U32,
        mc_x86_types::U64,
        mc_x86_types::NONE
    >;

    template <ir_opcode IrOpc, mc_x86_opcode X86Opc_ri, mc_x86_opcode X86Opc_rr>
    using sel_alu = pack<
        simple_rule<
            sel_comm<IrOpc, sel_cap<ir_opcode::IMM>, eat>,
            X86Opc_ri,
            copy_imm<0, 0>,
            copy_operand<1>,
            map_type<x86_target_type_lowering>
        >,
        n2n<IrOpc, X86Opc_rr, x86_target_type_lowering>
    >;

    using x86_isel_info_data = pack<                                                                          //
        sel_alu<ir::ir_opcode::ADD, mc_x86_opcode::ADD_ri, mc_x86_opcode::ADD_rr>,
        sel_alu<ir::ir_opcode::SUB, mc_x86_opcode::SUB_ri, mc_x86_opcode::SUB_rr>,
        simple_rule<
            sel_cap<ir::ir_opcode::IMM>,
            mc_x86_opcode::MOV_ri,
            map_type<x86_target_type_lowering>,
            copy_imm<0, 0>
        >
    >;
    // clang-format on

    struct x86_isel_info
    {
        using data = pack<x86_isel_info_data>;
    };

    struct x86_isel_pass : passes::isel::generic_isel<mc_x86_dag, x86_isel_info>
    {
        [[nodiscard]] auto pass_name() const -> std::string override { return "cg::x86::isel"; }
    };
} // namespace sbrt::x86
