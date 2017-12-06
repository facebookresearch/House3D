// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: camera.cc

#include <functional>

#include "camera.hh"
#include "gl/glfw++.hh"

#include "lib/debugutils.hh"

namespace {
constexpr int NR_KEYS = 1024;
}

namespace render {

void Camera::shift(Camera::Movement dir, float dist) {
  if (dir == FORWARD)
    pos += front * dist;
  if (dir == BACKWARD)
    pos -= front * dist;
  if (dir == LEFT)
    pos -= right * dist;
  if (dir == RIGHT)
    pos += right * dist;
  if (dir == UP)
    pos += up * dist;
  if (dir == DOWN)
    pos -= up * dist;
}

void Camera::turn(float dyaw, float dpitch) {
  yaw += dyaw;
  pitch += dpitch;
  pitch = glm::clamp(pitch, -89.f, 89.f);
  updateDirection();
}

CameraController::CameraController(GLFWwindow& window, Camera& cam):
  window_{window}, cam_(cam), keys_(NR_KEYS) {
    using namespace std::placeholders;

    glfwSetKeyCallback(&window, std::bind(
          &CameraController::keyCallback, this, _1, _2, _3, _4));

    glfwSetInputMode(&window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(&window, std::bind(
          &CameraController::mouseCallback, this, _1, _2));
    last_time_ = timer_.duration();
  }

void CameraController::keyCallback(int key, int /* scancode */, int action, int /* mode*/) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(&window_, GL_TRUE);
  if (key >= 0 && key < NR_KEYS) {
    if (action == GLFW_PRESS)
      keys_[key] = true;
    else if (action == GLFW_RELEASE)
      keys_[key] = false;
  }
}

void CameraController::mouseCallback(double x, double y) {
  if (!mouse_init_) {
    mouse_ = glm::vec2(x, y);
    mouse_init_ = true;
    return;
  }
  auto old = mouse_;
  mouse_ = glm::dvec2(x, y);
  auto offset = mouse_ - old;
  offset = offset * mouse_speed_;
  offset.y = -offset.y;   // mouse y coordinate and 3D y coordinate are opposite
  cam_.turn(offset.x, offset.y);
}

void CameraController::moveCamera() {
  float now = timer_.duration();
  float delta = now - last_time_;
  last_time_ = now;

  float dist = key_speed_ * delta;
  if (keys_[GLFW_KEY_W])
    cam_.shift(Camera::Movement::FORWARD, dist);
  if (keys_[GLFW_KEY_S])
    cam_.shift(Camera::Movement::BACKWARD, dist);
  if (keys_[GLFW_KEY_A])
    cam_.shift(Camera::Movement::LEFT, dist);
  if (keys_[GLFW_KEY_D])
    cam_.shift(Camera::Movement::RIGHT, dist);
  if (keys_[GLFW_KEY_UP])
    cam_.shift(Camera::Movement::UP, dist);
  if (keys_[GLFW_KEY_DOWN])
    cam_.shift(Camera::Movement::DOWN, dist);
  if (keys_[GLFW_KEY_LEFT])
    cam_.shift(Camera::Movement::LEFT, dist);
  if (keys_[GLFW_KEY_RIGHT])
    cam_.shift(Camera::Movement::RIGHT, dist);
}


}   // namespace render
