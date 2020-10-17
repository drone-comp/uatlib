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

struct unavailable { };
struct available { value_t min_value; };
struct owned { };

using permit_status = std::variant<unavailable, available, owned>;

// XXX: is it possible to use function_ref?
using bid_t = std::function<bool(const region&, uint_t, value_t)>;
using ask_t = std::function<bool(const region&, uint_t, value_t)>;
using status_t = std::function<permit_status(const region&, uint_t)>;

template <typename T>
using act_t = decltype(std::declval<T&>().act(uint_t{}, std::declval<bid_t>(), std::declval<status_t>(), int{}));

template <typename T>
using after_auction_t = decltype(std::declval<T&>().after_auction(uint_t{}, std::declval<ask_t>(), std::declval<status_t>(), int{}));

template <typename T>
using on_bought_t = decltype(std::declval<T&>().on_bought(std::declval<const region&>(), uint_t{}, value_t{}));

template <typename T>
using on_sold_t = decltype(std::declval<T&>().on_sold(std::declval<const region&>(), uint_t{}, value_t{}));

template <typename T>
using on_finished_t = decltype(std::declval<T&>().on_finished(uint_t{}, uint_t{}));

class agent
{
  class agent_interface
  {
    public:
      virtual ~agent_interface() = default;
      virtual auto clone() const -> std::unique_ptr<agent_interface> = 0;

      virtual auto act(uint_t, bid_t, status_t, int) -> bool = 0;
      virtual auto after_auction(uint_t, ask_t, status_t, int) -> void = 0;

      virtual auto on_bought(const region&, uint_t, value_t) -> void = 0;
      virtual auto on_sold(const region&, uint_t, value_t) -> void = 0;

      virtual auto on_finished(uint_t, uint_t) -> void = 0;
  };

  template <typename Agent>
  class agent_model : public agent_interface
  {
  public:
    agent_model(Agent agent) : agent_(std::move(agent)) {}
    virtual ~agent_model() = default;

    auto clone() const -> std::unique_ptr<agent_interface> override {
      return std::unique_ptr<agent_interface>{new agent_model(agent_)};
    }

    auto act(uint_t t, bid_t b, status_t i, int seed) -> bool override { return agent_.act(t, std::move(b), std::move(i), seed); }

    auto after_auction([[maybe_unused]] uint_t t, [[maybe_unused]] ask_t a, [[maybe_unused]] status_t i, [[maybe_unused]] int seed) -> void override
    {
      if constexpr (is_detected_exact_v<void, after_auction_t, Agent>)
        agent_.after_auction(t, std::move(a), std::move(i), seed);
    }

    auto on_bought([[maybe_unused]] const region& s, [[maybe_unused]] uint_t t, [[maybe_unused]] value_t v) -> void override
    {
      if constexpr (is_detected_exact_v<void, on_bought_t, Agent>)
        agent_.on_bought(s, t, v);
    }

    auto on_sold([[maybe_unused]] const region& s, [[maybe_unused]] uint_t t, [[maybe_unused]] value_t v) -> void override
    {
      if constexpr (is_detected_exact_v<void, on_sold_t, Agent>)
        agent_.on_sold(s, t, v);
    }

    auto on_finished([[maybe_unused]] uint_t id, [[maybe_unused]] uint_t t) -> void override
    {
      if constexpr (is_detected_exact_v<void, on_finished_t, Agent>)
        agent_.on_finished(id, t);
    }

  private:
    Agent agent_;
  };

public:
  template <typename Agent>
  agent(Agent a) : interface_(new agent_model<Agent>(std::move(a)))
  {
    static_assert(is_detected_convertible_v<bool, act_t, Agent>, "missing method 'Agent::act(t, bid, status, seed) -> Boolean'");
    assert(interface_);
  }

  agent(const agent& other);
  agent(agent&& other) noexcept = default;

  auto operator=(const agent& other) -> agent&;
  auto operator=(agent&& other) noexcept -> agent& = default;

  auto act(uint_t, bid_t, status_t, int) -> bool;
  auto after_auction(uint_t, ask_t, status_t, int) -> void;

  auto on_bought(const region&, uint_t, value_t) -> void;
  auto on_sold(const region&, uint_t, value_t) -> void;

  auto on_finished(uint_t, uint_t) -> void;

private:
  std::unique_ptr<agent_interface> interface_;
};

} // namespace uat

#endif // UAT_AGENT_HPP
