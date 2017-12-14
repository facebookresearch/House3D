// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: obj.hh

#pragma once

#include <unordered_map>
#include <vector>
#include "gl/api.hh"
#include <tiny_obj_loader.h>

#include "lib/mat.h"
#include "gl/geometry.hh"

namespace render {

class TextureRegistry;

// A structure to hold outputs from tinyobj
class ObjLoader {
  public:
    std::string base_dir;
    tinyobj::attrib_t attrib;
    // A vector of shapes. Might be more than the number of shapes in the obj,
    // because we split the shapes based on texture for rendering.
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    // Has the same length as `shapes`. Map index in `shapes` to its original shape index.
    std::vector<int> shape_ids;
    // Number of shapes originally in the obj.
    int original_num_shapes;

    ObjLoader(std::string fname) { load(fname); }

    void printInfo() const;

    // convert the faceid_th face in the mesh, to a TriangleFace
    TriangleFace convertFace(const tinyobj::mesh_t&, int faceid);

    void split_shapes_by_material();

    // sort shapes by transparency. Put opaque objects first.
    void sort_by_transparent(const TextureRegistry& tex);

  private:
    bool load(std::string fname);

};


// maintain mapping from texture name to registered OpenGL texture id
class TextureRegistry {
  public:
    TextureRegistry(
        const std::vector<tinyobj::material_t>& materials,
        std::string base_dir);

    TextureRegistry(const TextureRegistry&) = delete;
    TextureRegistry& operator = (const TextureRegistry&) = delete;
    TextureRegistry(TextureRegistry&&) = default;
    ~TextureRegistry() { deactivate(); }

    GLuint get(std::string texname) const {
      auto itr = map_.find(texname);
      if (itr == map_.end()) return 0;
      return itr->second;
    }

    bool is_transparent(std::string texname) const {
      auto itr = texture_images_.find(texname);
      if (itr == texture_images_.end()) return false;
      return itr->second.channels() == 4;
    }

    // populate map_ by texture_images_
    void activate();
    void deactivate();

  private:
    bool activated_ = false;
    // load a texture to OpenGL and return its texture id
    void loadTexture(const std::string& texname);

    // texname -> image, loaded and cached at the beginning
    std::unordered_map<std::string, Matuc> texture_images_;
    // texname -> opengl resource id
    std::unordered_map<std::string, GLuint> map_;
    std::string base_dir_;
};


} // namespace render
