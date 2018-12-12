// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.

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

