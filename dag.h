#pragma once

#include "cast.h"
#include "io.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>

class type_token
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

        PRIMITIVE_MAX
    };

private:
    std::uintptr_t data;

public:
    constexpr type_token(primitives primitive) : data(primitive) {}
    constexpr type_token() : data(NONE) {}

    [[nodiscard]] constexpr auto is_primitive() const -> bool { return data < PRIMITIVE_MAX; }
    [[nodiscard]] constexpr auto primitive() const -> primitives { return static_cast<primitives>(data); }
    [[nodiscard]] constexpr auto type_desc() const -> void* { return as_vptr(data); }

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

class dag;
class dag_node;

using dn_ref = dag_node*;

class dag_node
{
public:
    using imm_type = std::variant<uint64_t>;

private:
    dag* owner_dag;
    dn_ref chain_node;

    template <typename... Ts>
    constexpr void do_cons(dn_ref operand, Ts&&... rest)
    {
        operands.push_back(operand);
        do_cons(std::forward<Ts>(rest)...);
    }

    template <typename... Ts>
    constexpr void do_cons(imm_type&& curr_imm, Ts&&... rest)
    {
        imm.emplace_back(std::move(curr_imm));
        do_cons(std::forward<Ts>(rest)...);
    }

    constexpr void do_cons() {}

public:
    enum instr_opc
    {
        NONE,
        IMM,
    };

    template <typename... Ts>
        requires(((std::same_as<Ts, dn_ref> || std::same_as<Ts, imm_type>) && ...))
    constexpr dag_node(dag* dag, instr_opc opcode, type_token type, dn_ref chain = nullptr, Ts&&... args)
        : owner_dag(dag), chain_node(chain), opcode(opcode), type(type)
    {
        size_t op_count = ((std::same_as<Ts, dn_ref> ? 1 : 0) + ... + 0);
        operands.reserve(op_count);
        imm.reserve(sizeof...(Ts) - op_count);
        do_cons(std::forward<Ts>(args)...);
    }

    std::vector<dn_ref> operands;
    std::vector<imm_type> imm;
    instr_opc opcode;
    type_token type{};

    [[nodiscard]] constexpr auto get_chain() const -> dn_ref { return chain_node; }
    constexpr void set_chain(dn_ref new_chain)
    {
        assert(chain_node->owner_dag != owner_dag);
        chain_node = new_chain;
    }

    [[nodiscard]] constexpr auto get_tag() const -> dag* { return owner_dag; }
};

class dag
{
    std::vector<std::unique_ptr<dag_node>> nodes;

    template <typename... Ts>
    constexpr auto do_insert(dag_node::instr_opc opc, type_token type, dn_ref chain, Ts&&... value) -> dag_node*
    {
        return nodes.emplace_back(std::make_unique<dag_node>(this, opc, type, chain, std::forward<Ts>(value)...)).get();
    }

public:
    constexpr auto dummy() -> dag_node* { return do_insert(dag_node::NONE, type_token::NONE, nullptr); }
    constexpr auto load_imm(type_token type, uint64_t value) -> dag_node*
    {
        return do_insert(dag_node::IMM, type, nullptr, dag_node::imm_type(value));
    }
};
