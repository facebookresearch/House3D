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

#include "model/obj.hh"
#include "model/mesh.hh"
#include "model/scene.hh"
#include "gl/shader.hh"

#include "suncg/category.hh"
#include "suncg/color_mapping.hh"

namespace render {

class SUNCGShader: public Shader {
  public:
    SUNCGShader();

    static const char* fShader;
    GLint Kd_loc, Ka_loc, mode_loc,
          texture_loc, dissolve_loc, minDepth_loc;

    enum class RenderMode : GLuint {
      TEXTURE_LIGHTING = 0,
      LIGHTING = 1,
      CONSTANT = 2,
      DEPTH = 3,
      INVDEPTH = 4
    };
};

class SUNCGScene : public ObjSceneBase {
  public:
    explicit SUNCGScene(
        std::string obj_file,
        std::string model_category_file,
        std::string semantic_label_file,
        float minDepth = 0.3);
    ~SUNCGScene() {}

    void draw() override;
    void activate() override;
    void deactivate() override;

    Shader* get_shader() override { return &shader_; }

    enum class RenderMode {
      RGB = 0,
      SEMANTIC = 1,
      DEPTH = 2,
      INSTANCE = 3,
      INVDEPTH = 4
    };

    enum class ObjectNameResolution {
      COARSE = 0,   // use its coarse class name
      FINE = 1      // use its fine class name
    };

    void set_mode(RenderMode m) { mode_ = m; }

    void set_object_name_resolution_mode(ObjectNameResolution m)
    { object_name_mode_ = m; }

    RenderMode get_mode() const { return mode_; }

  protected:
    void parse_scene();

    std::string name_from_mode_id(std::string name) {
      // return the name used for rendering, from model id
      if (object_name_mode_ == ObjectNameResolution::COARSE) {
        return model_category_.get_coarse_grained_class(name);
      } else {
        return model_category_.get_fine_grained_class(name);
      }
    }

    glm::vec3 get_color_by_shape_name(const std::string& name);

    RenderMode mode_ = RenderMode::RGB;
    ObjectNameResolution object_name_mode_ = ObjectNameResolution::COARSE;
    SUNCGShader shader_;
    TextureRegistry textures_;

    ModelCategory model_category_;
    ColorMappingReader semantic_color_;
    glm::vec3 background_color_;
    std::vector<Mesh> mesh_;
    float minDepth_; // used for inverse depth mode

    struct MaterialDesc {
      int id;  // material id in tinyobj
      glm::vec3 label_color;
      glm::vec3 instance_color;
      GLuint texture;

      // doesn't own this pointer
      const tinyobj::material_t* m;   // the material with the texture field changing
    };
    // material for each mesh. Must have same size as mesh_
    std::vector<MaterialDesc> materials_;
};

} // namespace render
