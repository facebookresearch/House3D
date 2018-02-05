// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: scene.cc

#include "scene.hh"
#include <vector>
#include <iostream>
#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

#include "gl/utils.hh"
#include "gl/geometry.hh"
#include "lib/debugutils.hh"
#include "lib/timer.hh"
#include "lib/utils.hh"
#include "mesh.hh"
#include "shader.hh"

using namespace std;

namespace render {


SimpleObjScene::SimpleObjScene(string fname):
  ObjSceneBase{fname},
  textures_{obj_.materials, obj_.base_dir} {
    parse_scene();
    activate();
}

SimpleObjScene::SimpleObjScene(ObjLoader&& obj)
  : ObjSceneBase{move(obj)},
  textures_{obj_.materials, obj_.base_dir} {
    parse_scene();
    activate();
}

void SimpleObjScene::activate() {
  textures_.activate();
  int nr_mesh = mesh_.size();
  m_assert(nr_mesh == (int)materials_.size());

  for (int i = 0; i < nr_mesh; ++i) {
    MaterialDesc& material = materials_[i];
    if (material.m) {
      material.texture = textures_.get(material.m->diffuse_texname);
    }
    mesh_[i].activate();
  }
}

void SimpleObjScene::deactivate() {
  for (auto& m : mesh_)
    m.deactivate();
  textures_.deactivate();
}

void SimpleObjScene::parse_scene() {
  float x = std::numeric_limits<float>::max();
  boxmin_ = {x, x, x};
  x = std::numeric_limits<float>::lowest();
  boxmax_ = {x, x, x};
  obj_.split_shapes_by_material();
  obj_.printInfo();
  obj_.sort_by_transparent(textures_);

  for (auto& shp : obj_.shapes) {
    tinyobj::mesh_t& tmesh = shp.mesh;
    int nr_face = tmesh.num_face_vertices.size();
    auto& matids = tmesh.material_ids;
    m_assert((int)matids.size() == nr_face);
    m_assert(nr_face > 0);

    int mid = matids[0];

    tinyobj::material_t* matptr = mid >= 0 ? &obj_.materials.at(mid) : nullptr;

    mesh_.emplace_back();
    // Assume that obj_.materials won't change size any more
    materials_.emplace_back(MaterialDesc{mid, 0UL, matptr});

    for (int f = 0; f < nr_face; ++f) {
      auto face = obj_.convertFace(tmesh, f);
      for (auto& v : face) {
        mesh_.back().vertices.emplace_back(move(v));
        boxmin_ = glm::min(boxmin_, v.pos);
        boxmax_ = glm::max(boxmax_, v.pos);
      }
    }
  }
  obj_.shapes.clear();
  obj_.shapes.shrink_to_fit();
}

void SimpleObjScene::draw() {
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // TODO background color
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  int nr_mesh = mesh_.size();
  for (int i = 0; i < nr_mesh; ++i) {
    const auto& material = materials_[i];
    static_assert(
        std::is_same<std::decay<
        decltype(material.m->diffuse[0])>::type, GLfloat>::value,
      "tinyobj material type incompatible with GLfloat!");
    if (material.m) {
      glUniform3fv(shader_.Kd_loc, 1, (GLfloat*)&material.m->diffuse);
      glUniform3fv(shader_.Ka_loc, 1, (GLfloat*)&material.m->ambient);
      glUniform1f(shader_.dissolve_loc, material.m->dissolve);
    }

    auto mode = BasicShader::RenderMode::LIGHTING;
    if (material.texture) {
      glActiveTexture(GL_TEXTURE0);
      glUniform1i(shader_.texture_loc, 0);  // use TU0
      mode = BasicShader::RenderMode::TEXTURE_LIGHTING;
    }
    glUniform1ui(shader_.mode_loc, static_cast<GLuint>(mode));

    TextureGuard TG{material.texture};
    mesh_[i].draw();
  }
}

} // namespace render
