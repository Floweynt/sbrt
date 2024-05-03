#pragma once

#include "common.h"
#include "instr/ir.h"
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace sbrt::isel
{
    using namespace ir;

    namespace detail
    {
        template <typename T, ir_types::primitives Primitive>
        struct type_delegate_impl
        {
            static_assert(
                ((Primitive >= ir_types::U8 && Primitive <= ir_types::U64)) || Primitive == ir_types::PTR,
                "type selection type must be u8-u64 or ptr"
            );

            inline static constexpr size_t _eat_size = T::_eat_size;
            inline static constexpr auto _match(ir_dag_node* node) -> bool { return node->type.primitive() == Primitive && T::_match(node); }
            inline static constexpr auto _fast_match(ir_dag_node* node) -> bool
            {
                return node->type.primitive() == Primitive && T::_fast_match(node);
            }
        };

        template <typename T>
        concept sel_dag_node_matcher = requires(ir_dag_node* node) {
            {
                T::_match(node)
            } -> std::convertible_to<bool>;
            {
                T::_eat_size
            } -> std::convertible_to<size_t>;
        };

    } // namespace detail

    template <auto Bool, auto U8, auto U16, auto U32, auto U64, auto Ptr>
    struct target_type_lowering
    {
        inline static constexpr auto lower(ir_types type)
        {
            switch (type.primitive())
            {
            case ir_types::U8:
                return U8;
            case ir_types::U16:
                return U16;
            case ir_types::U32:
                return U32;
            case ir_types::U64:
                return U64;
            case ir_types::BOOL:
                return Bool;
            case ir_types::PTR:
                return Ptr;
            default:
                __builtin_unreachable();
            }
        }
    };

    /**
     * Consumes any node, and adds it to the argument list
     */
    struct eat
    {
        template <ir_types::primitives Primitive>
        using typed = detail::type_delegate_impl<eat, Primitive>;

        inline static constexpr size_t _eat_size = 1;
        inline static constexpr auto _match(ir_dag_node* /*unused*/) -> bool { return true; }

        template <size_t I, size_t N>
        inline static constexpr void _prepare_args(ir_dag_node* node, std::array<ir_dag_node*, N>& out)
        {
            out[I] = node;
        }
    };

    template <detail::sel_dag_node_matcher T>
    struct capture
    {
        inline static constexpr auto OPC = T::OPC;

        inline static constexpr size_t _eat_size = 1 + T::_eat_size;
        inline static constexpr auto _match(ir_dag_node* node) -> bool { return T::_match(node); }
        inline static constexpr auto _fast_match(ir_dag_node* node) -> bool { return T::_fast_match(node); }

        template <size_t I, size_t N>
        inline static constexpr void _prepare_args(ir_dag_node* node, std::array<ir_dag_node*, N>& out)
        {
            out[I] = node;
            T::template _prepare_args<I + 1>(node, out);
        }
    };

    struct skip
    {
        template <ir_types::primitives Primitive>
        using typed = detail::type_delegate_impl<eat, Primitive>;

        inline static constexpr size_t _eat_size = 0;
        inline static constexpr auto _match(ir_dag_node* /*unused*/) -> bool { return true; }

        template <size_t I>
        inline static constexpr void _prepare_args(ir_dag_node* /*node*/, auto& /*out*/)
        {
        }
    };

    /**
     * Selects an instruction with opcode = Opc, with operand constraints specified in Ts
     */
    template <ir_opcode Opc, detail::sel_dag_node_matcher... Ts>
    struct sel
    {
    private:
        template <size_t... Index>
        inline static constexpr auto _do_child_match(std::index_sequence<Index...> /*unused*/, ir_dag_node* instr) -> bool
        {
            return (Ts::_match(instr->operands[Index]) && ... && true);
        }

        template <size_t N, size_t I, size_t CI>
        inline static constexpr void _prepare_args_impl(std::vector<ir_dag_node*>& operands, std::array<ir_dag_node*, N>& out)
        {
        }

        // TODO: use fold expression to speed up compile
        template <size_t N, size_t I, size_t CI, detail::sel_dag_node_matcher V, detail::sel_dag_node_matcher... Rest>
        inline static constexpr void _prepare_args_impl(std::vector<ir_dag_node*>& operands, std::array<ir_dag_node*, N>& out)
        {
            V::_prepare_args<I>(operands[CI], out);
            _prepare_args_impl<N, I + V::_eat_size, CI + 1, Rest...>(operands, out);
        }

    public:
        template <ir_types::primitives Primitive>
        using typed = detail::type_delegate_impl<sel, Primitive>;

        inline static constexpr ir_opcode OPC = Opc;

        inline static constexpr size_t _eat_size = (Ts::_eat_size + ... + 0);

        inline static constexpr auto _match(ir_dag_node* instr) -> bool { return instr->opcode == OPC && _fast_match(instr); }

        inline static constexpr auto _fast_match(ir_dag_node* instr) -> bool
        {
            if (sizeof...(Ts) > instr->operands.size())
            {
                return false;
            }

            return _do_child_match(std::index_sequence_for<Ts...>{}, instr);
        }

        template <size_t I, size_t N>
        inline static constexpr void _prepare_args(ir_dag_node* node, std::array<ir_dag_node*, N>& out)
        {
            _prepare_args_impl<N, I, 0, Ts...>(node->operands, out);
        }
    };

    template <ir_opcode Opc, detail::sel_dag_node_matcher... Ts>
    using sel_cap = capture<sel<Opc, Ts...>>;

    template <ir_opcode opc, detail::sel_dag_node_matcher L, detail::sel_dag_node_matcher R>
    struct sel_comm
    {
        inline static constexpr ir_opcode OPC = opc;

        template <ir_types::primitives Primitive>
        using typed = detail::type_delegate_impl<sel_comm, Primitive>;

        inline static constexpr size_t _eat_size = (L::_eat_size + R::_eat_size);
        inline static constexpr auto _match(ir_dag_node* instr) -> bool { return instr->opcode == OPC && _fast_match(instr); }
        inline static constexpr auto _fast_match(ir_dag_node* instr) -> bool
        {
            return (L::_match(instr->operands[0]) && R::_match(instr->operands[1])) ||
                   (R::_match(instr->operands[0]) && L::_match(instr->operands[1]));
        }

        template <size_t I, size_t N>
        inline static constexpr void _prepare_args(ir_dag_node* instr, std::array<ir_dag_node*, N>& out)
        {
            if (L::_match(instr->operands[0]) && R::_match(instr->operands[1]))
            {
                L::template _prepare_args<I>(instr->operands[0], out);
                R::template _prepare_args<I + L::_eat_size>(instr->operands[1], out);
            }
            else
            {
                L::template _prepare_args<I>(instr->operands[1], out);
                R::template _prepare_args<I + L::_eat_size>(instr->operands[0], out);
            }
        }
    };

    template <ir_opcode opc, ir_types::primitives Type, detail::sel_dag_node_matcher L, detail::sel_dag_node_matcher R>
    using sel_comm_typed = typename sel_comm<opc, L, R>::template typed<Type>;

    template <typename Selector, auto Function>
    struct sel_rule
    {
        using match = Selector;
        inline static constexpr auto invoke = Function;
        inline static constexpr auto _expand = true;
    };

    template <ir_opcode Opc, auto TargetOpc, typename TargetTypeLowering>
    struct n2n
    {
        using match = sel<Opc>;

        inline static constexpr void invoke(ir_dag& /*ir_dag*/, ir_dag_node* ir_node, auto& dag, auto* target, const auto& /*args*/)
        {
            target->opcode = TargetOpc;
            if (ir_node->chain != nullptr)
            {
                target->chain = dag[ir_node->chain->get_id()];
            }

            for (auto* operand : ir_node->operands)
            {
                target->operands.push_back(dag[operand->get_id()]);
            }

            target->type = TargetTypeLowering::lower(ir_node->type);
        }

        inline static constexpr auto _expand = false;
    };

    template <size_t SelOpcI, size_t ImmIdx>
    struct copy_imm
    {
        inline static constexpr void _transform(ir_dag& /*ir_dag*/, ir_dag_node* /*ir_node*/, auto& /*dag*/, auto* target, const auto& args)
        {
            target->imm.push_back(variant_cast(args[SelOpcI]->imm[ImmIdx]));
        }
    };

    template <size_t SelOpcI>
    struct copy_operand
    {
        inline static constexpr void _transform(ir_dag& /*ir_dag*/, ir_dag_node* /*ir_node*/, auto& dag, auto* target, const auto& args)
        {
            target->operands.push_back(dag[args[SelOpcI]->get_id()]);
        }
    };

    template <typename Lowering>
    struct map_type
    {
        inline static constexpr void _transform(ir_dag& /*ir_dag*/, ir_dag_node* ir_node, auto& /*dag*/, auto* target, const auto& /*args*/)
        {
            target->type = Lowering::lower(ir_node->type);
        }
    };

    namespace detail
    {
        template <typename T>
        concept simple_tramsformer = requires() { T::_transform; };
    }

    template <typename Selector, auto NewOpc, typename... Transformations>
    struct simple_rule
    {
        using match = Selector;

        inline static constexpr void invoke(ir_dag& ir_dag, ir_dag_node* ir_node, auto& dag, auto* target, const auto& args)
        {
            target->opcode = NewOpc;

            (Transformations::_transform(ir_dag, ir_node, dag, target, args), ...);
        }

        inline static constexpr auto _expand = false;
    };
} // namespace sbrt::isel
