#include <uat/permit.hpp>

#include <boost/functional/hash.hpp>

namespace uat
{

region::region(const region& other) : interface_(other.interface_->clone()) {}

auto region::operator=(const region& other) -> region&
{
  interface_ = other.interface_->clone();
  return *this;
}

auto region::hash() const -> std::size_t { return interface_->hash(); }

auto region::operator==(const region& other) const -> bool { return interface_->equals(*other.interface_); }

auto region::operator!=(const region& other) const -> bool { return !interface_->equals(*other.interface_); }

auto region::print_to(std::function<void(std::string_view, fmt::format_args)> f) const -> void
{
  interface_->print_to(std::move(f));
}

permit::permit(region s, uint_t time) noexcept : region_{std::move(s)}, time_{time} {}

auto permit::time() const noexcept -> uint_t { return time_; }
auto permit::location() const noexcept -> const region& { return region_; }
auto permit::location() noexcept -> region& { return region_; }

auto permit::operator==(const permit& other) const -> bool { return location() == other.location() && time() == other.time(); }
auto permit::operator!=(const permit& other) const -> bool { return !(*this == other); }

} // namespace uat

namespace std
{
auto hash<uat::region>::operator()(const uat::region& s) const noexcept -> size_t { return s.hash(); }
auto hash<uat::permit>::operator()(const uat::permit& p) const noexcept -> size_t
{
  size_t seed = 0;
  boost::hash_combine(seed, p.location().hash());
  boost::hash_combine(seed, std::hash<std::size_t>{}(p.time()));
  return seed;
}
} // namespace std
