#ifndef UAT_AGENT_HPP
#define UAT_AGENT_HPP

#include <stdexcept>
#include <uat/type.hpp>

#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace uat
{

namespace permit_public_status
{
struct unavailable
{};
struct available
{
  value_t min_value;
};
struct owned
{};
} // namespace permit_public_status

using permit_public_status_t =
  std::variant<permit_public_status::unavailable, permit_public_status::available, permit_public_status::owned>;

// TODO: is it possible to use function_ref?
using bid_fn = std::function<bool(const region&, uint_t, value_t)>;
using ask_fn = std::function<bool(const region&, uint_t, value_t)>;
using permit_public_status_fn = std::function<permit_public_status_t(const region&, uint_t)>;

template <typename T>
using mb_act_t =
  decltype(std::declval<T&>().act(uint_t{}, std::declval<bid_fn>(), std::declval<permit_public_status_fn>(), int{}));

template <typename T>
using mb_after_trading_t =
  decltype(std::declval<T&>().after_trading(uint_t{}, std::declval<ask_fn>(), std::declval<permit_public_status_fn>(), int{}));

template <typename T>
using mb_on_bought_t = decltype(std::declval<T&>().on_bought(std::declval<const region&>(), uint_t{}, value_t{}));

template <typename T>
using mb_on_sold_t = decltype(std::declval<T&>().on_sold(std::declval<const region&>(), uint_t{}, value_t{}));

template <typename T> using mb_on_finished_t = decltype(std::declval<T&>().on_finished(uint_t{}, uint_t{}));

class agent
{
  class agent_interface
  {
  public:
    virtual ~agent_interface() = default;
    virtual auto clone() const -> std::unique_ptr<agent_interface> = 0;

    virtual auto act(uint_t, bid_fn, permit_public_status_fn, int) -> bool = 0;
    virtual auto after_trading(uint_t, ask_fn, permit_public_status_fn, int) -> void = 0;

    virtual auto on_bought(const region&, uint_t, value_t) -> void = 0;
    virtual auto on_sold(const region&, uint_t, value_t) -> void = 0;

    virtual auto on_finished(uint_t, uint_t) -> void = 0;
  };

  template <typename Agent> class agent_model : public agent_interface
  {
  public:
    agent_model(Agent agent) : agent_(std::move(agent)) {}
    virtual ~agent_model() = default;

    auto clone() const -> std::unique_ptr<agent_interface> override
    {
      return std::unique_ptr<agent_interface>{new agent_model(agent_)};
    }

    auto act(uint_t t, bid_fn b, permit_public_status_fn i, int seed) -> bool override
    {
      return agent_.act(t, std::move(b), std::move(i), seed);
    }

    auto after_trading([[maybe_unused]] uint_t t, [[maybe_unused]] ask_fn a, [[maybe_unused]] permit_public_status_fn i,
                       [[maybe_unused]] int seed) -> void override
    {
      if constexpr (is_detected_exact_v<void, mb_after_trading_t, Agent>)
        agent_.after_trading(t, std::move(a), std::move(i), seed);
    }

    auto on_bought([[maybe_unused]] const region& s, [[maybe_unused]] uint_t t, [[maybe_unused]] value_t v) -> void override
    {
      if constexpr (is_detected_exact_v<void, mb_on_bought_t, Agent>)
        agent_.on_bought(s, t, v);
    }

    auto on_sold([[maybe_unused]] const region& s, [[maybe_unused]] uint_t t, [[maybe_unused]] value_t v) -> void override
    {
      if constexpr (is_detected_exact_v<void, mb_on_sold_t, Agent>)
        agent_.on_sold(s, t, v);
    }

    auto on_finished([[maybe_unused]] uint_t id, [[maybe_unused]] uint_t t) -> void override
    {
      if constexpr (is_detected_exact_v<void, mb_on_finished_t, Agent>)
        agent_.on_finished(id, t);
    }

  private:
    Agent agent_;
  };

public:
  template <typename Agent> agent(Agent a) : interface_(new agent_model<Agent>(std::move(a)))
  {
    static_assert(is_detected_convertible_v<bool, mb_act_t, Agent>,
                  "missing function member Agent::act(...) -> convertible_to<bool>");
    assert(interface_);
  }

  agent(const agent& other);
  agent(agent&& other) noexcept = default;

  auto operator=(const agent& other) -> agent&;
  auto operator=(agent&& other) noexcept -> agent& = default;

  auto act(uint_t, bid_fn, permit_public_status_fn, int) -> bool;
  auto after_trading(uint_t, ask_fn, permit_public_status_fn, int) -> void;

  auto on_bought(const region&, uint_t, value_t) -> void;
  auto on_sold(const region&, uint_t, value_t) -> void;

  auto on_finished(uint_t, uint_t) -> void;

private:
  std::shared_ptr<agent_interface> interface_;
};

} // namespace uat

#endif // UAT_AGENT_HPP
