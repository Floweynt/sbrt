#pragma once

#include "common.h"
#include "instr/ir.h"
#include <expected.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sbrt
{
    template <typename T, typename U>
    class pass
    {
    public:
        using result_t = tl::expected<U, error>;

        [[nodiscard]] virtual auto pass_name() const -> std::string = 0;

        [[nodiscard]] virtual auto transform(T&& dag) const -> result_t = 0;

        virtual ~pass() = default;
    };

    template <typename T, typename U>
    using pass_ref = std::unique_ptr<pass<T, U>>;

    template <typename T>
    using transformer = pass<T, T>;

    template <typename T>
    using transformer_ref = pass_ref<T, T>;

    template <typename T>
    class pipeline final : public transformer<T>
    {
        std::vector<transformer_ref<T>> passes;

    public:
        pipeline(std::vector<transformer_ref<T>> passes) : passes(std::move(passes)) {}

        [[nodiscard]] auto pass_name() const -> std::string override
        {
            std::string result = "pipeline(";
            for (const auto& pass : passes)
            {
                result += pass.pass_name() + " ";
            }
            result.pop_back();
            result.push_back(')');
            return result;
        }

        auto transform(T&& dag) const -> transformer<T>::result_t override
        {
            T instance = std::move(dag);
            for (const auto& pass : passes)
            {
                auto temp = pass->transform();
                if (!temp)
                {
                    return temp;
                }

                instance = std::move(temp);
            }

            return instance;
        }

        ~pipeline() override = default;
    };

    class validate_ir_pass final : public transformer<ir::ir_dag>
    {
    public:
        [[nodiscard]] auto pass_name() const -> std::string override { return "dag_vaildate"; }

        auto transform(ir::ir_dag&& dag) const -> result_t override
        {
            // typecheck
            return std::move(dag);
        }
    };
} // namespace sbrt
