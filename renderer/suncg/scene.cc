// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: scene.cc

#include "scene.hh"
#include "model/shader.hh"

#include "category.hh"

using namespace std;

namespace {

std::vector<glm::vec3> get_uniform_sampled_colors(int count) {
  int interval_length = static_cast<int>(pow(256, 3)) / (count + 2);
  int current_color = interval_length;
  std::vector<glm::vec3> result;

  for (int i = 0; i < count; i++) {
    int r = current_color % 256;
    int g = (current_color / 256) % 256;
    int b = (current_color / 256 / 256) % 256;

    result.push_back(glm::vec3{r / 255.0, g / 255.0, b / 255.0});

    current_color += interval_length;
  }

  std::random_shuffle(result.begin(), result.end());
  return result;
}


}

namespace render {

const char* SUNCGShader::fShader = R"xxx(
#version 330 core

in vec3 pos;
in vec3 normal;
in vec2 texcoord;
out vec4 fragcolor;

uniform uint mode;
// 0: light + texture
// 1: light
// 2: const Kd
// 3: depth
uniform vec3 Kd;
uniform vec3 Ka;
uniform vec3 eye;
uniform float dissolve;
uniform sampler2D texture_diffuse;

// TODO change NEAR to 0.01
float near = 0.1;
float far  = 100.0;
float DEPTH_SCALE = 20.0;

// convert depth buffer value to true depth
// https://learnopengl.com/#!Advanced-OpenGL/Depth-testing
float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
    if (mode == 2u) { // constant
      fragcolor = vec4(Kd, 1.f);
      return;
    }
    if (mode == 3u) { // depth
      float depth = LinearizeDepth(gl_FragCoord.z) / DEPTH_SCALE;
      fragcolor = vec4(vec3(depth), 1.0);
      return;
    }

    float alpha = dissolve;
    vec3 color;
    switch(mode) {
      case 0u:
        vec4 texcolor = texture(texture_diffuse, texcoord);
        // for suncg, every face has Kd. Just multiply them.
        color = Kd * texcolor.xyz;
        alpha = min(texcolor.w, alpha);
        break;
      case 1u:
        color = Kd;
        break;
    }
    vec3 in_vec = normalize(eye - pos);
    // have some diffuse color even when orthogonal
    float scale = max(dot(in_vec, normal), 0.3f);
    vec3 ambient = Ka * 0.1;
    color = color * scale + ambient;
    color = clamp(color, 0.f, 1.f);
    fragcolor = vec4(color, alpha);
}
)xxx";

SUNCGShader::SUNCGShader():
  Shader{BasicShader::vShader, fShader} {

  Kd_loc = getUniformLocation("Kd");
  Ka_loc = getUniformLocation("Ka");
  mode_loc = getUniformLocation("mode");
  texture_loc = getUniformLocation("texture_diffuse");
  dissolve_loc = getUniformLocation("dissolve");
  };


SUNCGScene::SUNCGScene(string obj_file, string model_category_file,
    string semantic_label_file):
  ObjSceneBase{obj_file},
  textures_{obj_.materials, obj_.base_dir},
  model_category_{model_category_file},
  semantic_color_{semantic_label_file}
{
    background_color_ = semantic_color_.get_background_color();

    // use FINE_GRAINED if color mapping > 128
    if (semantic_color_.size() > 128)
      set_object_name_resolution_mode(ObjectNameResolution::FINE);

    // filter out person
    model_category_.filter_category(obj_.shapes, {"person"});
    // split shapes
    obj_.split_shapes_by_material();
    obj_.printInfo();
    obj_.sort_by_transparent(textures_);

    parse_scene();
    activate();
}

void SUNCGScene::activate() {
  textures_.activate();
  int nr_mesh = mesh_.size();
  m_assert(nr_mesh == (int)materials_.size());

  for (int i = 0; i < nr_mesh; ++i) {
    MaterialDesc& material = materials_[i];
    material.texture = textures_.get(material.m->diffuse_texname);
    mesh_[i].activate();
  }
}

void SUNCGScene::deactivate() {
  for (auto& m : mesh_)
    m.deactivate();
  textures_.deactivate();
}

glm::vec3 SUNCGScene::get_color_by_shape_name(const string& name) {
  if (name.find("Model#") == 0) {
    int size_prefix = 6;  // len(Model#)
    string model_id = name.substr(size_prefix);
    string klass = name_from_mode_id(model_id);
    glm::vec3 color = semantic_color_.get_color(klass);
    //print_debug("Shape %s, Class: %s -> color %f,%f,%f\n", name.c_str(), klass.c_str(), color.x, color.y, color.z);
    return color;
  } else if (name == "Ground") {
    return semantic_color_.get_color("Ground");
  } else {
    auto split = name.find('#');
    if (split != string::npos) {
      string klass = name.substr(0, split);

      if (klass == "WallInside" or klass == "WallOutside")
        klass = "Wall";
      glm::vec3 color = semantic_color_.get_color(klass);
      //print_debug("Shape %s, Class: %s -> color %f,%f,%f\n", name.c_str(), klass.c_str(), color.x, color.y, color.z);
      return color;
    }
  }
  print_debug("Failed to get color for shape %s\n", name.c_str());
  // TODO
  return background_color_;
}

void SUNCGScene::parse_scene() {
  float x = std::numeric_limits<float>::max();
  boxmin_ = {x, x, x};
  x = std::numeric_limits<float>::lowest();
  boxmax_ = {x, x, x};
  auto rand_instance_colors = get_uniform_sampled_colors(obj_.original_num_shapes);

  for (size_t i = 0; i < obj_.shapes.size(); i++) {
    auto& shp = obj_.shapes[i];
    glm::vec3 label_color = get_color_by_shape_name(shp.name);
    glm::vec3 instance_color = rand_instance_colors[shp.original_index];
    tinyobj::mesh_t& tmesh = shp.mesh;
    int nr_face = tmesh.num_face_vertices.size();
    auto& matids = tmesh.material_ids;
    m_assert((int)matids.size() == nr_face);
    m_assert(nr_face > 0);

    int mid = matids[0];
    mesh_.emplace_back();
    // Assume that obj_.materials won't change size any more
    materials_.emplace_back(MaterialDesc{mid, label_color, instance_color, 0UL, &obj_.materials[mid]});

    for (int f = 0; f < nr_face; ++f) {
      auto face = obj_.convertFace(tmesh, f);
      for (auto& v : face) {
        mesh_.back().vertices.emplace_back(move(v));
        boxmin_ = glm::min(boxmin_, v.pos);
        boxmax_ = glm::max(boxmax_, v.pos);
      }
    }
  }
  mesh_.shrink_to_fit();
  obj_.shapes.clear();
  obj_.shapes.shrink_to_fit();
}

void SUNCGScene::draw() {
  glClearColor(background_color_.x, background_color_.y, background_color_.z, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  int nr_mesh = mesh_.size();
  if (mode_ == RenderMode::RGB) {
    for (int i = 0; i < nr_mesh; ++i) {
      const auto& material = materials_[i];
      static_assert(
          std::is_same<std::decay<
          decltype(material.m->diffuse[0])>::type, GLfloat>::value,
          "tinyobj material type incompatible with GLfloat!");
      glUniform3fv(shader_.Kd_loc, 1, (GLfloat*)&material.m->diffuse);
      glUniform3fv(shader_.Ka_loc, 1, (GLfloat*)&material.m->ambient);
      glUniform1f(shader_.dissolve_loc, material.m->dissolve);

      auto mode = SUNCGShader::RenderMode::LIGHTING;
      if (material.texture) {
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(shader_.texture_loc, 0);  // use TU0
        mode = SUNCGShader::RenderMode::TEXTURE_LIGHTING;
      }
      glUniform1ui(shader_.mode_loc, static_cast<GLuint>(mode));

      TextureGuard TG{material.texture};
      mesh_[i].draw();
    }
  } else if (mode_ == RenderMode::SEMANTIC || mode_ == RenderMode::INSTANCE) {
    auto mode = SUNCGShader::RenderMode::CONSTANT;
    for (int i = 0; i < nr_mesh; ++i) {
      glm::vec3 color = mode_ == RenderMode::SEMANTIC ?
        materials_[i].label_color : materials_[i].instance_color;
      glUniform3fv(shader_.Kd_loc, 1, (GLfloat*)&color);
      glUniform1ui(shader_.mode_loc, static_cast<GLuint>(mode));
      mesh_[i].draw();
    }
  } else {    // depth
    auto mode = SUNCGShader::RenderMode::DEPTH;
    glUniform1ui(shader_.mode_loc, static_cast<GLuint>(mode));
    for (int i = 0; i < nr_mesh; ++i)
      mesh_[i].draw();
  }
}

}   // namespace render
