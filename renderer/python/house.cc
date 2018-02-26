//File: house.cc

#include "house.hh"

namespace py = pybind11;

namespace render {

void House::f(const std::vector<py::array>& numpy_arrays) {
  printf("%lu\n", numpy_arrays[0].size());
}

}
