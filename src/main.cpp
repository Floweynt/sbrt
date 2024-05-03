#include "arch/x86/instr.h"
#include "arch/x86/isel.h"
#include "common.h"
#include "dag_writer.h"
#include "instr/ir.h"
#include <bits/stl_algo.h>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <utility>

namespace
{
    void setup_handlers()
    {
        (void)signal(
            SIGABRT,
            +[](int) {
                sbrt::error("SIGABRT received", sbrt::submodule::MISC).print(std::cout);
                exit(-1);
            }
        );

        std::set_terminate([]() {
            try
            {
                std::rethrow_exception(std::current_exception());
            }
            catch (const sbrt::error& e)
            {
                e.print(std::cout);
            }
            catch (const std::exception& e)
            {
                sbrt::error(e.what(), sbrt::submodule::MISC).print(std::cout);
            }
            catch (...)
            {
                std::cerr << "Unhandled exception of unknown type" << '\n';
            }

            std::abort();
        });
    }
} // namespace

auto main() -> int
{
    setup_handlers();

    sbrt::ir::ir_dag dag;
    sbrt::ir::ir_dag_node* imm = dag.create(nullptr, sbrt::ir::ir_opcode::IMM, sbrt::ir::ir_types::U32, sbrt::ir::ir_imm_type(1Ul));

    dag.root(dag.create(
        nullptr, sbrt::ir::ir_opcode::ADD, sbrt::ir::ir_types::U32,
        dag.create(nullptr, sbrt::ir::ir_opcode::SUB, sbrt::ir::ir_types::U32, imm, imm),
        dag.create(nullptr, sbrt::ir::ir_opcode::SUB, sbrt::ir::ir_types::U32, imm, imm)
    ));

    sbrt::dag_dot_emitter<sbrt::ir::ir_instr_specific_info> ir_out{std::cout};
    dag.emit_dot(ir_out);
    sbrt::x86::x86_isel_pass pass;
    auto out = pass.transform(std::move(dag));

    sbrt::dag_dot_emitter<sbrt::x86::mc_x86_instr_specific_info> mc_out{std::cout};
    out->emit_dot(mc_out);
}
