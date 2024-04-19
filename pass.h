#include "common.h"
#include "lib/expected.h"
#include <expected>
#include <fmt/format.h>
#include <memory>
#include <ranges>
#include <string>

namespace views = std::ranges::views;

template <typename T>
class transform_pass
{
public:
    using result_t = tl::expected<T, error>;

    virtual auto pass_name() -> std::string = 0;

    virtual auto transform(T&& dag) -> result_t = 0;

    virtual ~transform_pass() = default;
};

template <typename T>
using pass_ref = std::unique_ptr<transform_pass<T>>;

template <typename T>
class pipeline : public transform_pass<T>
{
    std::vector<pass_ref<T>> passes;

public:
    auto pass_name() -> std::string override
    {
        return fmt::format("pipeline({})", views::transform([](const pass_ref<T>& x) { return x->pass_name(); }, ", "));
    }

    auto transform(T&& dag) -> transform_pass<T>::result_t override {}

    ~pipeline() override {}
};
