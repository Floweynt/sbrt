#pragma once

#include "instr/ir.h"
#include "pass/pass.h"
#include <string>

namespace sbrt::passes::isel
{
    class isel_ir_dag_check final : public transformer<ir::ir_dag>
    {
    public:
        [[nodiscard]] auto pass_name() const -> std::string override { return "cg::isel::dag_check"; }

        auto transform(ir::ir_dag&& _dag) const -> result_t override;
    };
} // namespace sbrt::passes::isel
