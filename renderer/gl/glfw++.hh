// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: glfw++.hh

#pragma once
#include <unordered_map>
#include <functional>

#include <GLFW/glfw3.h>

namespace {
std::unordered_map<GLFWwindow*,
  std::function<void(int, int)>> M2i;

std::unordered_map<GLFWwindow*,
  std::function<void(double, double)>> M2d;

std::unordered_map<GLFWwindow*,
  std::function<void(int, int, int, int)>> M4i;
}

namespace render {


void glfwSetWindowSizeCallback(GLFWwindow* window,
      std::function<void(int, int)> callback) {
    M2i[window] = std::move(callback);
    ::glfwSetWindowSizeCallback(window,
        [](GLFWwindow* wnd, int w, int h)
        { M2i[wnd](w, h); });
}

void glfwSetKeyCallback(GLFWwindow* window,
      std::function<void(int, int, int, int)> callback) {
    M4i[window] = std::move(callback);
    ::glfwSetKeyCallback(window,
        [](GLFWwindow* wnd, int a1, int a2, int a3, int a4)
        { M4i[wnd](a1, a2, a3, a4); });
}

void glfwSetCursorPosCallback(GLFWwindow* window,
      std::function<void(double, double)> callback) {
    M2d[window] = std::move(callback);
    ::glfwSetCursorPosCallback(window,
        [](GLFWwindow* wnd, double x, double y)
        { M2d[wnd](x, y); });
}

} // namespace render
