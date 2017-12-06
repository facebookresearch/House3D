// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: mesh.cc

#include "gl/api.hh"
#include "mesh.hh"
#include "gl/utils.hh"

using namespace std;

namespace render {

void Mesh::activate() {
  glGenVertexArrays(1, VAO);
  glGenBuffers(1, VBO);

  VertexArrayGuard VAG{VAO};
  // Load data into vertex buffers
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  // A great thing about structs is that their memory layout is sequential for all its items.
  // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
  // again translates to 3/2 floats which translates to a byte array.
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
      vertices.data(), GL_STATIC_DRAW);

  // Set the vertex attribute pointers
  // Vertex Positions
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
  // Vertex Normals
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, normal));
  // Vertex Texture Coords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, texcoord));
}

void Mesh::deactivate() {
  if (VAO)
    glDeleteVertexArrays(1, VAO);
  if (VBO)
    glDeleteBuffers(1, VBO);
}

void Mesh::draw() {
  VertexArrayGuard VAG{VAO};
  glDrawArrays(GL_TRIANGLES, 0, vertices.size());
  glCheckError("Mesh::draw::glDrawArrays");
}

}

