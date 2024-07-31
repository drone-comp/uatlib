//! \file agent.hpp
//! \brief Defines the agent class and related types.

#ifndef UAT_AGENT_HPP
#define UAT_AGENT_HPP

#include <uat/permit.hpp>

#include <span>
#include <variant>

#include <type_safe/reference.hpp>

namespace uat
{

//! Represents the public values in a trade of a permit.
struct trade_value_t
{
  value_t min_value;   //!< The minimum value the owner asked for the permit.
  value_t highest_bid; //!< The highest bid for the permit.
};

namespace permit_public_status
{

//! Represents the public status of a permit that is not available for trading.
struct unavailable
{};

//! \brief Represents the public status of a permit that is available for trading.
struct available
{
  value_t min_value; //!< The minimum value that can be offered for the permit.

  //! History of trades for the permit.
  //! Each element contains the minimum value and the highest bid.
  std::span<const trade_value_t> trades;
};

//! Represents the public status of a permit that is owned by the agent.
struct owned
{};

} // namespace permit_public_status

//! \brief Variant that represents the possible public status of a permit.
using permit_public_status_t =
  std::variant<permit_public_status::unavailable, permit_public_status::available, permit_public_status::owned>;

//! Function reference that allows the agent to bid for a permit.
using bid_fn = type_safe::function_ref<bool(region_view, uint_t, value_t)>;

//! Function reference that allows the agent to ask for a permit.
using ask_fn = type_safe::function_ref<bool(region_view, uint_t, value_t)>;

//! Function reference that returns the public status of a permit.
using permit_public_status_fn = type_safe::function_ref<permit_public_status_t(region_view, uint_t)>;

//! \brief Class to define the default behavior of an agent.
//!
//! All agent implementations should inherit from this class for a given
//! region type.  The region type should be a region_compatible type.
//! Functions that are not implemented by the agent will have no effect.
//! We suggest using `override` to ensure that no function signature mismatch
//! occurs.
template <region_compatible R> struct agent
{
  using region_type = R;

  //! Behavior of the agent during the bid phase.
  //!
  //! \param time The current time step.
  //! \param bid A function that allows the agent to bid for a permit.
  //! \param status A function that returns the public status of a permit.
  //! \param seed A random seed.
  //!
  //! The function `bid` and `status` receive a `region_compatible` and a `uint_t` as arguments
  //! representing the location and time of the permit.  The third argument of `bid`
  //! is the value the agent is willing to bid for the permit. The function `bid` returns
  //! true if the bidding was successful, and false otherwise.  The function `status`
  //! returns the public status of the permit with type `permit_public_status_t`.
  //!
  //! \note The default behavior of this function is to do nothing.  We suggest
  //!       using `override` to ensure that no function signature mismatch occurs.
  virtual auto bid_phase([[maybe_unused]] uint_t time, [[maybe_unused]] bid_fn bid,
                         [[maybe_unused]] permit_public_status_fn status, [[maybe_unused]] int seed) -> void
  {}

  //! Behavior of the agent during the ask phase.
  //!
  //! \param time The current time step.
  //! \param ask A function that allows the agent to ask for a permit.
  //! \param status A function that returns the public status of a permit.
  //! \param seed A random seed.
  //!
  //! The function `ask` and `status` receive a `region` and a `uint_t` as arguments
  //! representing the location and time of the permit.  The third argument of `ask`
  //! is the value the agent is willing to ask for the permit. The function `ask` returns
  //! true if the asking was successful, and false otherwise.  The function `status`
  //! returns the public status of the permit with type `permit_public_status_t`.
  //!
  //! \note The default behavior of this function is to do nothing.  We suggest
  //!       using `override` to ensure that no function signature mismatch occurs.
  virtual auto ask_phase([[maybe_unused]] uint_t time, [[maybe_unused]] ask_fn ask,
                         [[maybe_unused]] permit_public_status_fn status, [[maybe_unused]] int seed) -> void
  {}

  //! Callback function called when the agent successfully buys a permit.
  //!
  //! \param region The region of the permit.
  //! \param time The time of the permit.
  //! \param value The value paid for the permit.
  //!
  //! \note The default behavior of this function is to do nothing.  We suggest
  //!       using `override` to ensure that no function signature mismatch occurs.
  virtual auto on_bought([[maybe_unused]] const R& region, [[maybe_unused]] uint_t time, [[maybe_unused]] value_t value) -> void
  {}

  //! Callback function called when the agent successfully sells a permit.
  //!
  //! \param region The region of the permit.
  //! \param time The time of the permit.
  //! \param value The value received for the permit.
  //!
  //! \note The default behavior of this function is to do nothing.  We suggest
  //!       using `override` to ensure that no function signature mismatch occurs.
  virtual auto on_sold([[maybe_unused]] const R& region, [[maybe_unused]] uint_t time, [[maybe_unused]] value_t value) -> void {}

  //! Controls when the agent should stop.
  //!
  //! Once this function returns true, the agent will be removed from the simulation.
  //!
  //! \param time The current time step.
  //! \param seed A random seed.
  //!
  //! \note This function must be implemented by the agent.
  virtual auto stop(uint_t time, int seed) -> bool = 0;
};

//! \brief Concept that defines the requirements for an agent.
template <typename T>
concept agent_compatible = std::movable<T> && std::derived_from<T, agent<typename T::region_type>>;

//! \brief A type-erased class that represents an agent in the simulation.
//!
//! Any type that inherits from `agent` can be converted to an `any_agent`.
class any_agent
{
  //! \private
  class agent_interface
  {
  public:
    virtual ~agent_interface() = default;

    virtual auto bid_phase(uint_t, bid_fn, permit_public_status_fn, int) -> void = 0;
    virtual auto ask_phase(uint_t, ask_fn, permit_public_status_fn, int) -> void = 0;

    virtual auto on_bought(region_view, uint_t, value_t) -> void = 0;
    virtual auto on_sold(region_view, uint_t, value_t) -> void = 0;

    virtual auto stop(uint_t, int) -> bool = 0;
  };

  //! \private
  template <agent_compatible Agent> class agent_model : public agent_interface
  {
  public:
    agent_model(Agent any_agent) : agent_(std::move(any_agent)) {}
    virtual ~agent_model() = default;

    auto bid_phase(uint_t t, bid_fn b, permit_public_status_fn i, int seed) -> void override
    {
      agent_.bid_phase(t, std::move(b), std::move(i), seed);
    }

    auto ask_phase(uint_t t, ask_fn a, permit_public_status_fn i, int seed) -> void override
    {
      agent_.ask_phase(t, std::move(a), std::move(i), seed);
    }

    auto on_bought(region_view s, uint_t t, value_t v) -> void override
    {
      agent_.on_bought(s.downcast<typename Agent::region_type>(), t, v);
    }

    auto on_sold(region_view s, uint_t t, value_t v) -> void override
    {
      agent_.on_sold(s.downcast<typename Agent::region_type>(), t, v);
    }

    auto stop(uint_t t, int seed) -> bool override { return agent_.stop(t, seed); }

  private:
    Agent agent_;
  };

public:
  //! Constructs a type-erased any_agent from an object of type that inherits from `agent`
  //! and is movable.
  template <agent_compatible Agent> any_agent(Agent a) : interface_(new agent_model<Agent>(std::move(a))) {}

  any_agent() = delete;

  any_agent(const any_agent&) = delete;
  any_agent(any_agent&&) noexcept = default;

  auto operator=(const any_agent&) -> any_agent& = delete;
  auto operator=(any_agent&&) noexcept -> any_agent& = default;

  auto bid_phase(uint_t, bid_fn, permit_public_status_fn, int) -> void;
  auto ask_phase(uint_t, ask_fn, permit_public_status_fn, int) -> void;

  auto on_bought(region_view, uint_t, value_t) -> void;
  auto on_sold(region_view, uint_t, value_t) -> void;

  auto stop(uint_t time, int seed) -> bool;

private:
  std::unique_ptr<agent_interface> interface_;
};

} // namespace uat

#endif // UAT_AGENT_HPP
