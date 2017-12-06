// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: geometry.hh

#pragma once

#include <array>
#include <glm/glm.hpp>

namespace render {

// Vertex with pos, normal and texcoord
struct Vertex final {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 texcoord;

  Vertex(): pos{0}, normal{0}, texcoord{0} {}
};

typedef std::array<Vertex, 3> TriangleFace;

// compute normal direction of a triangle
inline glm::vec3 calcNormal(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
  auto v10 = v1 - v0,
       v20 = v2 - v0;
  glm::vec3 norm = glm::cross(v10, v20);
  return glm::normalize(norm);
}


} // namespace render
