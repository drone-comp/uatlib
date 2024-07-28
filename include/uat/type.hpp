#ifndef UAT_TYPE_HPP
#define UAT_TYPE_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace uat
{

//! Default unsigned integer type
using uint_t = std::size_t;

//! Default type for identifier
using id_t = std::size_t;

//! Default type for price
using value_t = double;

class airspace;
class region;
template <typename> class permit;
class agent;

class not_implemented : public std::exception
{
public:
  not_implemented(const std::string& member_name)
    : message("member function " + member_name + " not implemented")
  {}

  const char* what() const noexcept override { return message.c_str(); }
private:
  std::string message;
};

//! \private
namespace detail
{

//! \private
template <class Default, class AlwaysVoid, template <class...> class Op, class... Args> struct detector
{
  using value_t = std::false_type;
  using type = Default;
};

//! \private
template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
{
  using value_t = std::true_type;
  using type = Op<Args...>;
};

} // namespace detail

//! \private
struct nonesuch
{
  ~nonesuch() = delete;
  nonesuch(nonesuch const&) = delete;
  void operator=(nonesuch const&) = delete;
};

//! \private
template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

//! \private
template <template <class...> class Op, class... Args> constexpr bool is_detected_v = is_detected<Op, Args...>::value;

//! \private
template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

//! \private
template <class Expected, template <class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

//! \private
template <class Expected, template <class...> class Op, class... Args>
constexpr bool is_detected_exact_v = is_detected_exact<Expected, Op, Args...>::value;

//! \private
template <class To, template <class...> class Op, class... Args>
using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, To>;

//! \private
template <class To, template <class...> class Op, class... Args>
constexpr bool is_detected_convertible_v = is_detected_convertible<To, Op, Args...>::value;

//! \private
template <typename T> using equality_t = decltype(std::declval<const T&>() == std::declval<const T&>());

} // namespace uat

#endif // UAT_TYPE_HPP
