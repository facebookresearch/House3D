// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: render.hh

#pragma once
#include <string>
#include <memory>
#include <utility>
#include <future>
#include <queue>
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>

#include "scene.hh"
#include "gl/fbScope.hh"
#include "gl/glContext.hh"
#include "gl/camera.hh"
#include "model/scenecache.hh"
#include "lib/executor.hh"

namespace render {

// An instance of this class has to be created and used in the same thread.
// If not, use SUNCGRenderAPIThread.
class SUNCGRenderAPI {
  public:
    SUNCGRenderAPI(int w, int h, int device)
      : context_(render::createHeadlessContext(Geometry{w, h}, device)),
      geo_{w, h}, fb_{geo_} {
        // enable the common context options
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
      }

    // Load the scene objects to GPU, and unload current scene if it exists.
    // obj_file: house.obj in SUNCG
    // model_category_file: path to ModelCategoryMapping.csv
    // semantic_label_file: path to colormap_coarse.csv or colormap_fine.csv
    void loadScene(
        std::string obj_file, std::string model_category_file,
        std::string semantic_label_file);

    void setMode(SUNCGScene::RenderMode m) { scene_->set_mode(m); }

    // Render the image. The return format is:
    // For RGB mode, returns a 3-channel RGB image.
    // For semantic mode, returns a 3-channel image. The mapping from color to class is in the CSV.
    // For instance mode, returns a 3-channel image.
    //  Each unique color means an instance and the coloring is consistent across different views.
    //
    // For depth mode, returns a 2-channel image.
    //
    // The first channel contains depth values scaled to (0, 255),
    // where pixel / 255.0 * 20.0 is the true depth in meters.
    // Any pixels that are further than 20 meters away will be clipped to 20 meters.
    //
    // The second channel is an "infinity" mask. Each value is either 0 or 255.
    // 255 means this pixel is at infinty depth, and the correspoding first channel is meaningless.
    Matuc render();

    // Render a cube map of size 6w * h * c.  See render() for rendering details.
    // Cube map orientations are { BACK, LEFT, FORWARD, RIGHT, UP, DOWN }
    Matuc renderCubeMap();

    // Print OpenGL context info.
    void printContextInfo() const { context_->printInfo(); }

    // Get the camera. Caller doesn't own pointer
    // Caller can use the returned camera to move around the scene.
    // After the loading a new scene, you'll need to call getCamera() again
    // to get the camera in the new scene.
    Camera* getCamera() const { return camera_.get(); }

    // Get the resolution.
    Geometry resolution() const { return geo_; }

  private:
    SceneCache scene_cache_;
    SUNCGScene* scene_ = nullptr; // no ownership

    std::unique_ptr<GLContext> context_;
    std::unique_ptr<Camera> camera_;
    Geometry geo_;
    Framebuffer fb_;

    // set camera "smartly" to some place in the scene
    void init_camera_() {
      auto range = scene_->get_range();
      auto mid = scene_->get_min() + range * 0.5f;
      mid.z += glm::compMax(range);
      camera_.reset(new Camera{mid});
    }
};


// Same as SUNCGRenderAPI.
// But delegates all methods to run on an independent thread.
class SUNCGRenderAPIThread {
  public:
    SUNCGRenderAPIThread(int w, int h, int device) {
      exec_.execute_sync([=]() {
            this->api_.reset(new SUNCGRenderAPI{w, h, device});
          });
    }

    ~SUNCGRenderAPIThread() {
      exec_.execute_sync([=]() { api_.reset(nullptr); });
      exec_.stop();
    }

    void printContextInfo() {
      exec_.execute_sync([=]() {
            this->api_->printContextInfo();
          } );
    }

    // caller doesn't own pointer
    Camera* getCamera() const { return api_->getCamera(); }
    void setMode(SUNCGScene::RenderMode m) { api_->setMode(m); }
    Geometry resolution() const { return api_->resolution(); }

    void loadScene(
        std::string obj_file, std::string model_category_file,
        std::string semantic_label_file) {
      exec_.execute_sync([=]() {
            this->api_->loadScene(obj_file, model_category_file, semantic_label_file);
          });
    }

    Matuc render() {
      return exec_.execute_sync<Matuc>([=]() { return this->api_->render(); });
    }

    Matuc renderCubeMap() {
      return exec_.execute_sync<Matuc>([=]() {
        return this->api_->renderCubeMap();
      });
    }

  private:
    std::unique_ptr<SUNCGRenderAPI> api_;
    ExecutorInThread exec_;
};

}

