// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: scene.cc



#include "scene.hh"

#include "gl/api.hh"

#include "gl/shader.hh"
#include "gl/utils.hh"
#include "lib/debugutils.hh"
#include "lib/utils.hh"

namespace {

const GLchar* vertexShaderSource = R"xxx(
#version 330 core
layout (location = 0) in vec3 position;
void main() {
  gl_Position = vec4(position.x, position.y, position.z, 1.0);
}
)xxx";

const GLchar* fragmentShaderSource = R"xxx(
#version 330 core
out vec4 color;
void main() {
  color = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)xxx";
}

namespace render {

void RectangleScene::init() {
    // Build and compile our shader program
    // Vertex shader
    shader_.reset(
        new Shader{vertexShaderSource, fragmentShaderSource});

    GLfloat vertices[] = {
         0.5f,  0.5f, 0.0f,  // Top Right
         0.5f, -0.5f, 0.0f,  // Bottom Right
        -0.5f, -0.5f, 0.0f,  // Bottom Left
        -0.5f,  0.5f, 0.0f   // Top Left
    };
    GLuint indices[] = {  // Note that we start from 0!
        0, 1, 3,  // First Triangle
        1, 2, 3   // Second Triangle
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
    VertexArrayGuard VAG{VAO};

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind
}

void RectangleScene::draw() {
    // Clear the colorbuffer
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw our first triangle
    shader_->use();
    VertexArrayGuard VAG{VAO};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

RectangleScene::~RectangleScene() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

} // namespace
