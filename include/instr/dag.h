#pragma once

#include "dag_writer.h"
#include <cassert>
#include <cstddef>
#include <memory>
#include <queue>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace sbrt
{
    template <typename OpcodeType, typename TypeType, typename ImmType, typename ThisType, typename NodeType>
    class dag;

    template <typename OpcodeType, typename TypeType, typename ImmType, typename ThisType, typename DagType>
    class dag_node
    {
    public:
        using dag_type = DagType;
        friend class dag<OpcodeType, TypeType, ImmType, DagType, ThisType>;

    private:
        dag_type* owner_dag;
        size_t id;

        constexpr void do_cons() {}

        template <typename... Ts>
        constexpr void do_cons(ThisType* operand, Ts&&... rest)
        {
            operands.push_back(operand);
            do_cons(std::forward<Ts>(rest)...);
        }

        template <typename... Ts>
        constexpr void do_cons(ImmType&& curr_imm, Ts&&... rest)
        {
            imm.emplace_back(std::move(curr_imm));
            do_cons(std::forward<Ts>(rest)...);
        }

    public:
        template <typename... Ts>
            requires(((std::same_as<std::decay_t<Ts>, ThisType*> || std::same_as<std::decay_t<Ts>, ImmType>) && ...))
        constexpr dag_node(dag_type* dag, size_t node_id, ThisType* chain, OpcodeType opcode, TypeType type, Ts&&... args)
            : owner_dag(dag), id(node_id), chain(chain), opcode(opcode), type(type)
        {
            size_t op_count = ((std::same_as<Ts, ThisType*> ? 1 : 0) + ... + 0);
            operands.reserve(op_count);
            imm.reserve(sizeof...(Ts) - op_count);
            do_cons(std::forward<Ts>(args)...);
        }

        std::vector<ThisType*> operands;
        ThisType* chain;
        std::vector<ImmType> imm;
        OpcodeType opcode;
        TypeType type{};

        [[nodiscard]] constexpr auto get_dag() const -> auto* { return owner_dag; }
        [[nodiscard]] constexpr auto get_id() const -> size_t { return id; }
    };

    template <typename OpcodeType, typename TypeType, typename ImmType, typename ThisType, typename NodeType>
    class dag
    {
    public:
        using node_type = NodeType;
        using opcode_type = OpcodeType;
        using type_type = TypeType;

    private:
        std::vector<std::unique_ptr<node_type>> nodes;
        node_type* root_ptr;

    public:
        template <typename... Ts>
        constexpr auto create(node_type* chain, OpcodeType opcode, TypeType type, Ts&&... args) -> node_type*
        {
            return nodes.emplace_back(std::make_unique<node_type>((ThisType*)this, nodes.size(), chain, opcode, type, std::forward<Ts>(args)...))
                .get();
        }

        auto max_node_id() -> size_t { return nodes.size(); }

        auto root() -> node_type* { return root_ptr; }
        auto root(node_type* node)
        {
            assert(node->owner_dag == (ThisType*)this);
            root_ptr = node;
        }

        void visit_nodes(auto callback)
        {
            std::queue<node_type*> queue;
            queue.push(root());
            std::vector<bool> visited(max_node_id());

            while (!queue.empty())
            {
                auto node = queue.front();
                queue.pop();

                if (visited[node->get_id()])
                {
                    continue;
                }

                callback(node);
                visited[node->get_id()] = true;
                for (auto* operand : node->operands)
                {
                    queue.push(operand);
                }
            }
        }

        auto emit_dot(auto& output)
        {
            visit_nodes([&output](node_type* node) {
                output.emit_node(node, "");

                if (node->chain != nullptr)
                {
                    output.emit_edge(node, "chain", node->chain, dag_edge_type::CHAIN);
                }

                for (int i = 0; i < node->operands.size(); i++)
                {
                    output.emit_edge(node, std::to_string(i) + ":n", node->operands[i], dag_edge_type::OPERAND);
                }
            });

            output.done(root());
        }

        constexpr auto get_nodes() -> std::span<std::unique_ptr<node_type>> { return nodes; }
        constexpr auto get_nodes() const -> std::span<const std::unique_ptr<node_type>> { return nodes; }
        auto operator[](size_t node_id) -> node_type* { return nodes[node_id].get(); }
        auto operator[](size_t node_id) const -> const node_type* { return nodes[node_id].get(); }
    };
} // namespace sbrt

#define build_instruction_set(name)                                                                                                                  \
    struct name##_dag;                                                                                                                               \
    struct name##_dag_node : dag_node<name##_opcode, name##_types, name##_imm_type, name##_dag_node, name##_dag>                                     \
    {                                                                                                                                                \
        using dag_node<name##_opcode, name##_types, name##_imm_type, name##_dag_node, name##_dag>::dag_node;                                         \
    };                                                                                                                                               \
                                                                                                                                                     \
    struct name##_dag : dag<name##_opcode, name##_types, name##_imm_type, name##_dag, name##_dag_node>                                               \
    {                                                                                                                                                \
        using dag<name##_opcode, name##_types, name##_imm_type, name##_dag, name##_dag_node>::dag;                                                   \
    };
