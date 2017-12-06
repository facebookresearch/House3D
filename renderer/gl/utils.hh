// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: utils.hh

#pragma once

#include "api.hh"
#include <glm/vec3.hpp>
#include <string>
#include <iostream>

#include "lib/debugutils.hh"
#include "lib/strutils.hh"
namespace render {

inline void glCheckError(std::string msg) {
  GLenum e = glGetError();
  if (e != GL_NO_ERROR) {
    error_exit(ssprintf(
        "OpenGL error in \"%s\": %d (%d)\n", msg.c_str(), e, e));
  }
}

struct VertexArrayGuard {
  VertexArrayGuard(GLuint vao) { glBindVertexArray(vao); }
  ~VertexArrayGuard() { glBindVertexArray(0); }
};

struct TextureGuard {
  TextureGuard(GLuint tid)
  { glBindTexture(GL_TEXTURE_2D, tid); }
  ~TextureGuard() { glBindTexture(GL_TEXTURE_2D, 0); }
};


// A movable wrapper around a GLuint resource,
// whose ownership cannot be shared
template<typename IntType>
struct GLIntResource {
  using type = IntType;
  GLIntResource(): obj{0} {}
  GLIntResource(IntType o): obj{o} {}
  GLIntResource(const GLIntResource&) = delete;
  GLIntResource& operator =(const GLIntResource&) = delete;
  GLIntResource(GLIntResource&& r):
    obj{r.obj} { r.obj = 0; }
  GLIntResource& operator = (GLIntResource&& r) {
    obj = r.obj;
    r.obj = 0;
  }

  IntType obj = 0;

  // some implicit conversion
  operator IntType() const { return obj; }
  operator IntType*() { return &obj; }
  operator bool() const { return obj; }
};

void printGLDebugMsg(GLuint num);
bool tryEnableGLDebug();

bool checkExtension(const std::string& ext);

inline std::ostream& operator << (std::ostream& os, glm::vec3& v) {
  os << "(" << v.x << "," << v.y << "," << v.z << ")";
  return os;
}

} // namespace render
