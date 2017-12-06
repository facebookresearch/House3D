// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: scene.hh


#pragma once

#include <memory>
#include "gl/api.hh"

#include "gl/shader.hh"

namespace render {

// a test scene
class RectangleScene {
  public:
    RectangleScene() { init(); }

    void draw();

    ~RectangleScene();

  protected:
    void init();

    std::unique_ptr<Shader> shader_;
    GLuint VAO, VBO, EBO;
};

} // namespace
