#pragma once

#include "common.h"
#include "instr/dag.h"
#include "instr/ir.h"
#include "pass/pass.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace sbrt::passes::isel
{
    using namespace ir;

    class ir_to_isel final : public transformer<ir_dag>
    {
    public:
        [[nodiscard]] auto pass_name() const -> std::string override { return "cg::ir::lower_to_isel"; }

        [[nodiscard]] auto transform(ir_dag&& dag) const -> result_t override
        {
            for (auto& node : dag.get_nodes())
            {
                if (node->type.is_ptr())
                {
                    node->type = ir_type_token::PTR;
                }
            }

            return std::move(dag);
        }
    };

    namespace detail
    {
        template <typename T>
        struct handler_entry
        {
            bool (*match)(ir_dag_node*);
            void (*invoke)(ir_dag& ir_dag, ir_dag_node* ir_node, T& dag, T::node_type* target);
        };

        template <typename T>
        using sel_table_t = std::array<std::vector<handler_entry<T>>, static_cast<size_t>(ir_opcode::MAX)>;

        template <typename T, typename Selector>
        struct dummy
        {
            template <size_t... Index>
            static void expand_invoke(
                std::index_sequence<Index...> /*ignored*/, ir_dag& ir_dag, ir_dag_node* ir_node, T& dag, T::node_type* target,
                std::array<ir_dag_node*, Selector::match::_eat_size>& arr
            )
            {
                Selector::invoke(ir_dag, ir_node, dag, target, dag[arr[Index].get_id()]...);
            }
        };

        template <typename T, typename... Selectors>
        struct dummy<T, pack<Selectors...>>
        {
        };

        template <typename T, typename... Args>
        auto operator+(sel_table_t<T>& lhs, dummy<T, pack<Args...>> /*rhs*/) -> sel_table_t<T>&
        {
            (lhs + ... + dummy<T, Args>{});
            return lhs;
        }

        template <typename T, typename Selector>
        auto operator+(sel_table_t<T>& lhs, dummy<T, Selector> /*rhs*/) -> sel_table_t<T>&
        {
            using node_type = T::node_type;
            if constexpr (Selector::_expand)
            {
                lhs[static_cast<size_t>(Selector::match::OPC)].push_back({
                    Selector::match::_fast_match,
                    +[](ir_dag& ir_dag, ir_dag_node* ir_node, T& dag, node_type* target) {
                        std::array<ir_dag_node*, Selector::match::_eat_size> buffer;
                        Selector::match::template _prepare_args<0>(ir_node, buffer);
                        expand_invoke(std::make_index_sequence<Selector::match::_eat_size>{}, ir_dag, ir_node, dag, target, buffer);
                    },
                });
            }
            else
            {
                lhs[static_cast<size_t>(Selector::match::OPC)].push_back({
                    Selector::match::_fast_match,
                    +[](ir_dag& ir_dag, ir_dag_node* ir_node, T& dag, node_type* target) {
                        std::array<ir_dag_node*, Selector::match::_eat_size> buffer;
                        Selector::match::template _prepare_args<0>(ir_node, buffer);
                        Selector::invoke(ir_dag, ir_node, dag, target, buffer);
                    },
                });
            }
            return lhs;
        }
    } // namespace detail

    template <typename T, typename Info>
    class generic_isel : public pass<ir_dag, T>
    {
        using node_type = T::node_type;

        template <typename... Selectors>
        static auto build_sel_table(pack<Selectors...> /*inst*/) -> auto
        {
            detail::sel_table_t<T> res;
            (res + ... + detail::dummy<T, Selectors>{});
            return res;
        }

        detail::sel_table_t<T> selector_table = build_sel_table(typename Info::data{});

    public:
        auto transform(ir_dag&& _dag) const -> pass<ir_dag, T>::result_t override
        {
            ir_dag dag = std::move(_dag);
            T new_dag;
            for (size_t i = 0; i < dag.max_node_id(); i++)
            {
                new_dag.create(nullptr, T::opcode_type::NONE, T::type_type::NONE);
            }

            for (size_t i = 0; i < dag.max_node_id(); i++)
            {
                for (const auto& sel : selector_table[static_cast<uint64_t>(dag[i]->opcode)])
                {
                    ir_dag_node* node = dag[i];
                    if (sel.match(node))
                    {
                        sel.invoke(dag, node, new_dag, new_dag[i]);
                        break;
                    }
                }
            }

            new_dag.root(new_dag[dag.root()->get_id()]);

            return std::move(new_dag);
        }
    };
} // namespace sbrt::passes::isel
