// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: render.cc

#include "render.hh"

#include "gl/fbScope.hh"
#include "lib/imgproc.hh"

namespace render {


Matuc SUNCGRenderAPI::render() {
  FramebufferScope fb{fb_};
  Shader* shader_ = scene_->get_shader();
  shader_->use();
  shader_->setMat4("projection", camera_->getCameraMatrix(geo_));
  shader_->setVec3("eye", camera_->pos);

  scene_->draw();

  auto buf = fb.capture();
  if (scene_->get_mode() == SUNCGScene::RenderMode::DEPTH) {
    Matuc ret(geo_.h, geo_.w, 2);
    fill(ret, (unsigned char)0);
    for (int i = 0; i < geo_.h; ++i) {
      unsigned char* destptr = ret.ptr(i);
      for (int j = 0; j < geo_.w; ++j) {
        unsigned char* ptr = buf.ptr(i, j);
        if (ptr[0] == ptr[1] and ptr[1] == ptr[2])
          destptr[j * 2] = ptr[0];
        else
          destptr[j * 2 + 1] = 255;
      }
    }
    return ret;
  } else {
    return buf;
  }
}


Matuc SUNCGRenderAPI::renderCubeMap() {
  float prev_fov = camera_->vertical_fov;
  float prev_pitch = camera_->pitch;
  camera_->pitch = 0.f;
  camera_->vertical_fov = 90.f;

  // Start with back view
  std::vector<Matuc> faces;
  camera_->turn(180.f, 0.f);
  faces.push_back(render());

  // Render left, front, and right views
  for (int i = 1; i < 4; i++) {
    camera_->turn(90.f, 0.f);
    faces.push_back(render());
  }
  camera_->turn(-90.f, 0.f);

  // Render the top and bottom views
  camera_->turn(0.f, 89.f);
  faces.push_back(render());

  camera_->turn(0.f, 2 * -89.f);
  faces.push_back(render());

  // Reset camera
  camera_->vertical_fov = prev_fov;
  camera_->pitch = prev_pitch;
  camera_->updateDirection();

  return hconcat(faces);
}

void SUNCGRenderAPI::loadScene(
    std::string obj_file, std::string model_category_file,
    std::string semantic_label_file) {
  // check cache for previously loaded scenes
  scene_ = dynamic_cast<SUNCGScene*>(scene_cache_.get(obj_file));
  if (!scene_) {
    scene_ = new SUNCGScene{obj_file, model_category_file, semantic_label_file};
    scene_cache_.put(obj_file, scene_);
  }
  init_camera_();
}

}
