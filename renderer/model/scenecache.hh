// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: scenecache.hh

#pragma once

#include <unordered_map>
#include <string>

#include "scene.hh"

namespace render {

// Manage cache scenes, as well as the activating scenes.
class SceneCache {
  public:
    SceneCache() {}
    ~SceneCache() {
      for (auto& pair: cached_scenes_)
        delete pair.second;
    }
    SceneCache(const SceneCache&) = delete;
    SceneCache& operator =(const SceneCache&) = delete;


    // will deactivate current scene, activate new scene
    // Caller doesn't own the return pointer.
    ObjSceneBase* get(const std::string& name) {
      auto itr = cached_scenes_.find(name);
      if (itr != cached_scenes_.end()) {
        ObjSceneBase* new_scene = itr->second;
        if (new_scene != scene_) {
          scene_->deactivate();
          new_scene->activate();
          scene_ = new_scene;
        }
        return scene_;
      }
      return nullptr;
    }

    // ptr must be activated already.
    // This will deactivate old scene, put the pair into cache
    // The caller transfer ownership to SceneCache.
    void put(const std::string& name, ObjSceneBase* ptr) {
      if (scene_)
        scene_->deactivate();
      scene_ = ptr;
      cached_scenes_[name] = ptr;
    }

  private:
    // cache previously loaded scenes
    // This hash owns all the pointers.
    std::unordered_map<std::string, ObjSceneBase*> cached_scenes_;

    // The current scene
    ObjSceneBase* scene_ = nullptr;  // doesn't own this pointer. Only this scene should be activated.
};

}

