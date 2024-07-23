#include <pybind11/embed.h>
#include <pybind11/operators.h>

#include <uat/permit.hpp>
#include <uat/agent.hpp>

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(uat, m) {
  py::class_<uat::region>(m, "Region")
    .def("distance", &uat::region::distance)
    .def(py::self == py::self);
}

namespace uat
{

class py_agent {
public:
  template <typename... Args>
  explicit py_agent(py::object Agent, Args&&... args) {
    agent = Agent(std::forward<Args>(args)...);
  }

  auto bid_phase(uint_t, bid_fn, permit_public_status_fn, int) -> void;
  auto ask_phase(uint_t, ask_fn, permit_public_status_fn, int) -> void;

  auto on_bought(const region&, uint_t, value_t) -> void;
  auto on_sold(const region&, uint_t, value_t) -> void;

  auto stop(uint_t, int) -> bool;

private:
  py::object agent;
};

} // namespace uat
