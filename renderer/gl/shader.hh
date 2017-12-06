// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: shader.hh

#pragma once

#include <string>
#include <iostream>

#include "api.hh"
#include <glm/glm.hpp>

#include "lib/strutils.hh"
#include "lib/debugutils.hh"

namespace render {


class Shader {
  public:
    // Constructor generates the shader on the fly
    Shader(const char* vertexShader, const char* fragmentShader) {
      // 2. Compile shaders
      GLint success;
      GLchar infoLog[512];
      // Vertex Shader
      auto vertex = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(vertex, 1, &vertexShader, NULL);
      glCompileShader(vertex);
      // Print compile errors if any
      glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        error_exit(ssprintf(
              "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog));
      }
      // Fragment Shader
      auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(fragment, 1, &fragmentShader, NULL);
      glCompileShader(fragment);
      // Print compile errors if any
      glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        error_exit(ssprintf(
              "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog));
      }
      // Shader Program
      this->Program = glCreateProgram();
      glAttachShader(this->Program, vertex);
      glAttachShader(this->Program, fragment);
      glLinkProgram(this->Program);
      // Print linking errors if any
      glGetProgramiv(this->Program, GL_LINK_STATUS, &success);
      if (!success) {
        glGetProgramInfoLog(this->Program, 512, NULL, infoLog);
        error_exit(ssprintf(
              "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog));
      }
      // Delete the shaders as they're linked into our program now and no longer necessery
      glDeleteShader(vertex);
      glDeleteShader(fragment);
    }

    void use() const { glUseProgram(Program); }

    GLint getUniformLocation(const char* name) const
    { return glGetUniformLocation(Program, name); }

    void setMat4(const char* name, const glm::mat4 &mat) const {
      glUniformMatrix4fv(
          glGetUniformLocation(Program, name), 1, GL_FALSE, &mat[0][0]);
    }

    void setVec3(const char* name, const glm::vec3& vec) const {
      glUniform3fv(
          glGetUniformLocation(Program, name), 1, (const GLfloat*)&vec);
    }

  protected:
    GLuint Program;
};

} // namespace
