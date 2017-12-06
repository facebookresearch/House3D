// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: shader.hh

#pragma once


#include "gl/api.hh"
#include <glm/glm.hpp>
#include "gl/shader.hh"

namespace render  {

// Basic direct lighting with texture
class BasicShader : public Shader {
  public:
    enum class RenderMode : GLuint {
      TEXTURE_LIGHTING = 0,
      LIGHTING = 1
    };

    BasicShader();

    GLint Kd_loc, Ka_loc, mode_loc,
          texture_loc, dissolve_loc;

    static const char *vShader, *fShader;
};

}

