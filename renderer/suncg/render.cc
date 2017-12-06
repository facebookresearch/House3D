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
