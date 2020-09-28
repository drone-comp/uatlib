#include <uat/slot.hpp>

#include <boost/functional/hash.hpp>

namespace uat
{

slot::slot(const slot& other) : interface_(other.interface_->clone()) {}

auto slot::operator=(const slot& other) -> slot&
{
  interface_ = other.interface_->clone();
  return *this;
}

auto slot::neighbors() const -> std::vector<slot> { return interface_->neighbors(); }

auto slot::hash() const -> std::size_t { return interface_->hash(); }

auto slot::operator==(const slot& other) const -> bool { return interface_->equals(*other.interface_); }

auto slot::operator!=(const slot& other) const -> bool { return !interface_->equals(*other.interface_); }

auto slot::distance(const slot& other) const -> uint_t { return interface_->distance(*other.interface_); }

auto slot::heuristic_distance(const slot& other) const -> value_t { return interface_->heuristic_distance(*other.interface_); }

auto slot::shortest_path(const slot& other, int seed) const -> std::vector<slot> { return interface_->shortest_path(*other.interface_, seed); }

auto slot::print_to(std::function<void(std::string_view, fmt::format_args)> f) const -> void {
  interface_->print_to(std::move(f));
}

auto slot::turn(const slot& before, const slot& to) const -> bool { return interface_->turn(*before.interface_, *to.interface_); }

auto slot::climb(const slot& to) const -> bool { return interface_->climb(*to.interface_); }

tslot::tslot(class slot s, uint_t time) noexcept : slot_{std::move(s)}, time_{time} {}

auto tslot::time() const noexcept -> uint_t { return time_; }
auto tslot::slot() const noexcept -> const class slot& { return slot_; }
auto tslot::slot() noexcept -> class slot& { return slot_; }

auto tslot::operator==(const tslot& other) const -> bool { return slot() == other.slot() && time() == other.time(); }
auto tslot::operator!=(const tslot& other) const -> bool { return !(*this == other); }

} // namespace uat

namespace std
{
auto hash<uat::slot>::operator()(const uat::slot& s) const noexcept -> size_t {
  return s.hash();
}
auto hash<uat::tslot>::operator()(const uat::tslot& p) const noexcept -> size_t {
  size_t seed = 0;
  boost::hash_combine(seed, p.slot().hash());
  boost::hash_combine(seed, std::hash<std::size_t>{}(p.time()));
  return seed;
}
} // namespace std

