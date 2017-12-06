// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: mesh.hh\2

#pragma once
#include <vector>
#include <array>

#include "gl/geometry.hh"
#include "gl/utils.hh"

namespace render {

// Mesh is a bunch of vertices (usaully triangles)
class Mesh {
  public:
    std::vector<Vertex> vertices;

    Mesh() {}
    Mesh(const Mesh&) = delete;
    Mesh& operator = (const Mesh&) = delete;
    Mesh(Mesh&&) = default;

    ~Mesh() { deactivate(); }

    // setup GL buffers for rendering
    void activate();
    void deactivate();
    void draw();
  protected:
    GLIntResource<GLuint> VAO, VBO;
};

} // namespace render

