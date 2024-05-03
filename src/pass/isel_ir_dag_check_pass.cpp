#include "pass/isel_ir_dag_check_pass.h"
#include "magic_enum.hpp"
#include "common.h"
#include "instr/ir.h"
#include <fmt/core.h>
#include <utility>

auto sbrt::passes::isel::isel_ir_dag_check::transform(ir::ir_dag&& _dag) const -> result_t
{
    ir::ir_dag dag = std::move(_dag);

    // validate types
    for (auto& node : dag.get_nodes())
    {
        if (!node->type.is_primitive())
        {
            error("Illegal non-primitive type in ISel DAG", submodule::ISEL).add_note("In pass " + pass_name()).do_throw();
        }

        switch (node->type.primitive())
        {
        case ir::ir_types::U8:
        case ir::ir_types::U32:
        case ir::ir_types::U16:
        case ir::ir_types::U64:
        case ir::ir_types::BOOL:
        case ir::ir_types::PTR:
            break;
        default:
            error(fmt::format("Illegal primitive type in ISel DAG {}", magic_enum::enum_name(node->type.primitive())), submodule::ISEL)
                .add_note("In pass " + pass_name())
                .do_throw();
        }
    }

    return dag;
}
