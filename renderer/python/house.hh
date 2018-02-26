//File: house.hh

#pragma once

#include <pybind11/numpy.h>


namespace render {

// Implement some methods about houses that are too slow to do in python
class House {
  typedef pybind11::array nparray;
  public:
    void f(const std::vector<nparray>& numpy_arrays);

};

}

