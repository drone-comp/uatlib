#ifndef UAT_TYPE_HPP
#define UAT_TYPE_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace uat
{

using uint_t = std::size_t;
using id_t = std::size_t;
using value_t = double;

class airspace;
class region;
class permit;
class agent;

namespace detail
{

template <class Default, class AlwaysVoid, template <class...> class Op, class... Args> struct detector
{
  using value_t = std::false_type;
  using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
{
  using value_t = std::true_type;
  using type = Op<Args...>;
};

} // namespace detail

struct nonesuch
{
  ~nonesuch() = delete;
  nonesuch(nonesuch const&) = delete;
  void operator=(nonesuch const&) = delete;
};

template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args> constexpr bool is_detected_v = is_detected<Op, Args...>::value;

template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Expected, template <class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

template <class Expected, template <class...> class Op, class... Args>
constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;

template <class To, template <class...> class Op, class... Args>
using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, To>;

template <class To, template <class...> class Op, class... Args>
constexpr bool is_detected_convertible_v = is_detected_convertible<To, Op, Args...>::value;

template <typename T> using equality_t = decltype(std::declval<const T&>() == std::declval<const T&>());

} // namespace uat

#endif // UAT_TYPE_HPP
