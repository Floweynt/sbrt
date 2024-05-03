#pragma once
#include <cstdint>
#include <magic_enum.hpp>
#include <ostream>
#include <string>
#include <string_view>

namespace sbrt
{
    enum class dag_edge_type
    {
        OPERAND,
        CHAIN
    };

    template <typename InstrSpecificInfo>
    class dag_dot_emitter
    {
        std::ostream& output;

    public:
        dag_dot_emitter(std::ostream& out) : output(out)
        {
            out << "digraph G {\n";
            out << "node [shape=record,style=rounded]\nedge [dir=\"back\"]\n";
        }

        void emit_edge(auto* from, const std::string& port, auto* to, dag_edge_type kind)
        {
            output << "node" << (uintptr_t)to << ":opc:s -> node" << (uintptr_t)from << ":" << port;

            switch (kind)
            {
            case dag_edge_type::OPERAND:
                break;
            case dag_edge_type::CHAIN:
                output << " [style=dotted,color=blue]";
            }

            output << "\n";
        }

        void emit_node(auto* instr, std::string_view extra_label)
        {
            output << "node" << std::to_string((uintptr_t)instr) << " [label=\"{";

            if (instr->operands.size() != 0 || instr->chain != nullptr)
            {
                output << "{";

                if (instr->chain != nullptr)
                {
                    output << "<chain>ch|";
                }

                for (int i = 0; i < instr->operands.size(); i++)
                {
                    std::string operand_name = InstrSpecificInfo::get_operand_name(instr, i);
                    if (operand_name.empty())
                    {
                        operand_name = std::to_string(i);
                    }

                    output << "<" << i << ">" << operand_name;
                    if (i + 1 != instr->operands.size())
                    {
                        output << "|";
                    }
                }

                output << "}|";
            }

            for (int i = 0; i < instr->imm.size(); i++)
            {
                output << "<i" << i << ">[I]" << InstrSpecificInfo::get_imm_name(instr, i) << ": " << InstrSpecificInfo::serialize_imm(instr->imm[i])
                       << "|";
            }

            if (!extra_label.empty())
            {
                output << extra_label << "|";
            }

            output << "T:" << InstrSpecificInfo::serialize_type(instr->type) << "|<opc>" << magic_enum::enum_type_name<decltype(instr->opcode)>()
                   << "::" << magic_enum::enum_name(instr->opcode) << "}\"]\n";
        }

        void done(auto* root)
        {
            output << "node" << (uintptr_t)root << " -> InstrRoot [style=dotted]\n";
            output << "}\n";
        }
    };
} // namespace sbrt
