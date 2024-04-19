#include "bytecode.h"
#include "dag.h"
#include "io.h"
#include <cstddef>
#include <stack>
#include <string>
#include <utility>

using namespace sbrt;
namespace
{
    auto read_type(byte_reader& reader) -> type_token
    {
        auto res = reader.read_uleb();
        if (res < type_token::PRIMITIVE_MAX)
        {
            return {static_cast<type_token::primitives>(res)};
        }

        throw io_error("failed to read type token: " + std::to_string(res));
    }

    dn_ref read_one_instr(byte_reader& reader, dag& dag)
    {
        auto opc = reader.read_uleb();

        // this is an opcode
        switch (opc)
        {
        case dag_node::IMM: {
            // leaf node
            auto type = read_type(reader);
            auto imm = type.read_uint(reader);
            return dag.load_imm(type, imm);
        }
            // we don't know what opc this is...
        default:
            break;
        }

        throw io_error("bytecode: failed to read instr, unknown opcode " + std::to_string(opc));
    }

    void read_dag(byte_reader& reader)
    {
        // basically, we have to keep reading instructions
        // we keep track of stack<dn_ref, size_t left>
        std::stack<std::pair<dn_ref, size_t>> stack;

        while () {}
    }

} // namespace
void bytecode_file::read(byte_reader& reader)
{
    if (reader.read_u32() != MAGIC)
    {
        throw io_error("failed to read file: illegal magic");
    }

    major = reader.read_u16();
    minor = reader.read_u16();
    size_t constant_table_size = reader.read_uleb();
    size_t function_table_size = reader.read_uleb();
    size_t extra_table_size = reader.read_uleb();

    // read constants
    constant_table.resize(constant_table_size);
    for (auto& entry : constant_table)
    {
        entry.flags = reader.read_u8();
        entry.size = reader.read_uleb();
        if (!(entry.flags & constant_table_entry::RESERVE_ONLY))
        {
            entry.data = reader.read(entry.size);
        }
    }

    // TODO: read type table

    // read functions
    function_table.resize(function_table_size);

    for (size_t index = 0; index < function_table.size(); index++)
    {
        auto& entry = function_table[index];
        entry.flags = reader.read_u32();
        if (entrypoint_index != -1ULL && entry.flags & function_table_entry::ENTRYPOINT)
        {
            entrypoint_index = index;
        }

        read_dag(reader);
    }
}
