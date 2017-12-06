// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: scene.hh

#pragma once

#include <string>
#include <vector>

#include "gl/api.hh"
#include <glm/glm.hpp>

#include <tiny_obj_loader.h>
#include "obj.hh"
#include "mesh.hh"
#include "shader.hh"

namespace render {

class ObjSceneBase {
  public:
    explicit ObjSceneBase(std::string fname):
      obj_{fname} {}
    explicit ObjSceneBase(ObjLoader&& obj):
      obj_{std::move(obj)} {}
    virtual ~ObjSceneBase() {};

    virtual void draw() = 0;

    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual Shader* get_shader() = 0;

    glm::vec3 get_range() const
    { return boxmax_ - boxmin_; }

    glm::vec3 get_min() const
    { return boxmin_; }

  protected:
    glm::vec3 boxmin_, boxmax_;
    ObjLoader obj_;
};

// A basic rendering of an obj file, with direct lighting.
class SimpleObjScene : public ObjSceneBase {
  public:
    // The shader reference will be kept for use throughout the lifetime of the scene
    explicit SimpleObjScene(std::string fname);
    explicit SimpleObjScene(ObjLoader&& obj);
    ~SimpleObjScene() {}

    void draw() override;
    void activate() override;
    void deactivate() override;
    Shader* get_shader() override { return &shader_; }

  protected:
    void parse_scene();

    BasicShader shader_;
    TextureRegistry textures_;
    std::vector<Mesh> mesh_;

    struct MaterialDesc {
      int id;  // material id in tinyobj
      GLuint texture;

      // doesn't own this pointer
      const tinyobj::material_t* m;   // the material with the texture field changing
    };
    // material for each mesh. Must have same size as mesh_
    std::vector<MaterialDesc> materials_;
};

} // namespace render
