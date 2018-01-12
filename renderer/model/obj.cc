// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: obj.cc

#include "obj.hh"

#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <glm/gtc/type_ptr.hpp>

#include "lib/debugutils.hh"
#include "lib/strutils.hh"
#include "lib/utils.hh"
#include "lib/timer.hh"
#include "lib/imgproc.hh"

using namespace std;

namespace {

string getBaseDir(const string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

} // namespace

namespace render {

bool ObjLoader::load(string fname) {
  //TotalTimer tm("ObjLoader::load");
  base_dir = getBaseDir(fname);
#ifdef _WIN32
  base_dir += "\\";
#else
  base_dir += "/";
#endif

  string err;
  vector<tinyobj::shape_t> tmp_shapes;
  bool ret = tinyobj::LoadObj(&attrib, &tmp_shapes, &materials, &err, fname.c_str(), base_dir.c_str());
  if (not ret)
    error_exit(err);
  // if (not err.empty()) {
  //   cerr << "Warnings from tinyobj:" << endl;
  //   cerr << err << endl;
  // }

  original_num_shapes = tmp_shapes.size();
  shapes.reserve(tmp_shapes.size());
  for (size_t i = 0; i < tmp_shapes.size(); i++) {
    auto& shp = tmp_shapes[i];
    shapes.emplace_back(Shape{
        std::move(shp.mesh), std::move(shp.name), static_cast<int>(i)});
  }

  return true;
}

void ObjLoader::printInfo() const {
  printf("# of vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
  printf("# of normals   = %d\n", (int)(attrib.normals.size()) / 3);
  printf("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
  printf("# of materials = %d\n", (int)materials.size());
  printf("# of shapes    = %d\n", (int)shapes.size());
}

TriangleFace ObjLoader::convertFace(
    const tinyobj::mesh_t& mesh, int faceid) {

  m_assert(mesh.num_face_vertices[faceid] == 3);
  tinyobj::index_t idx[3] = {
    mesh.indices[faceid * 3],
    mesh.indices[faceid * 3 + 1],
    mesh.indices[faceid * 3 + 2] };

  TriangleFace face;
  bool has_normal = true;
  for (int vi = 0; vi < 3; ++vi) {
    face[vi].pos = glm::make_vec3(
        &attrib.vertices.at(3 * idx[vi].vertex_index));
    if (idx[vi].normal_index >= 0) {
      face[vi].normal = glm::make_vec3(
          &attrib.normals.at(3 * idx[vi].normal_index));
    } else {
      has_normal = false;
    }
    if (idx[vi].texcoord_index >= 0)
      face[vi].texcoord = glm::make_vec2(
          &attrib.texcoords.at(2 * idx[vi].texcoord_index));
  }

  if (!has_normal) {
    glm::vec3 norm = calcNormal(face[0].pos, face[1].pos, face[2].pos);
    for (int vi = 0; vi < 3; ++vi)
      face[vi].normal = norm;
  }
  return face;
}

void ObjLoader::split_shapes_by_material() {
  vector<Shape> new_shapes;
  vector<int> new_shape_ids;

  for (size_t i = 0; i < shapes.size(); i++) {
    auto& shp = shapes[i];
    tinyobj::mesh_t& tmesh = shp.mesh;

    int nr_face = tmesh.num_face_vertices.size();
    auto& matids = tmesh.material_ids;

    // I don't know what are tags. Assert for now.
    m_assert(tmesh.tags.size() == 0);
    m_assert((int)matids.size() == nr_face);

    if (nr_face == 0) continue;

    // mesh needs to be grouped by their material id
    // material id -> shape id in new_shapes
    unordered_map<int, int> mesh_by_mat;

    for (int f = 0; f < nr_face; ++f) {
      // TODO only works for triangle for now
      m_assert(tmesh.num_face_vertices[f] == 3);
      int mid = matids[f];

      auto itr = mesh_by_mat.find(mid);

      tinyobj::mesh_t* new_mesh;
      if (itr != mesh_by_mat.end()) {
        new_mesh = &new_shapes[itr->second].mesh;
      } else {
        mesh_by_mat[mid] = new_shapes.size();
        new_shapes.emplace_back();
        new_shapes.back().name = shp.name;
        new_shapes.back().original_index = shp.original_index;
        new_mesh = &new_shapes.back().mesh;
      }
      for (int k = 0; k < 3; ++k)
        new_mesh->indices.emplace_back(tmesh.indices[3 * f + k]);
      new_mesh->num_face_vertices.emplace_back(3);
      new_mesh->material_ids.emplace_back(mid);
      // ignore mesh_t::tags
    }
  }
  print_debug("Split shapes by material: %lu -> %lu\n", shapes.size(), new_shapes.size());
  std::swap(shapes, new_shapes);
}

void ObjLoader::sort_by_transparent(const TextureRegistry& tex) {
  auto is_transparent_material = [this,&tex](int matid) {
     auto& m = this->materials[matid];
     if (m.dissolve < 1.0)
       return true;
     if (m.diffuse_texname.empty())
       return false;
     return tex.is_transparent(m.diffuse_texname);
  };


  // Get the sorted indices for the shapes
  std::sort(shapes.begin(), shapes.end(),
      [this, &is_transparent_material](const Shape& a, const Shape& b) -> bool {
        bool is_a = is_transparent_material(a.mesh.material_ids[0]),
             is_b = is_transparent_material(b.mesh.material_ids[0]);
        if (is_a != is_b)
          return is_b;
        return a.name.compare(b.name) < 0;
  });
}

TextureRegistry::TextureRegistry(
    const vector<tinyobj::material_t>& materials,
    string base_dir): base_dir_(base_dir) {
  for (size_t i = 0; i < materials.size(); i++) {
    auto& m = materials[i];
    string texname = m.diffuse_texname;
    if (texname.empty()) continue;
    if (get(texname) == 0)
      loadTexture(texname);

    if (m.specular_texname.length() or m.normal_texname.length()
        or m.specular_highlight_texname.length() or m.ambient_texname.length()) {
      print_debug("Material %s has unsupported texture!\n", m.name.c_str());
    }
  }
}


void TextureRegistry::loadTexture(const string& texname) {
  string filename = squeeze_path(texname);
  if (!exists_file(texname.c_str())) {
    // Append base dir.
    filename = squeeze_path(base_dir_ + texname);
    if (!exists_file(filename.c_str()))
      error_exit(ssprintf("Cannot find texture %s\n", texname.c_str()));
  }

  Matuc image = read_img(filename.c_str());
  vflip(image);
  m_assert(image.channels() >= 3);
  texture_images_[texname] = std::move(image);
}

void TextureRegistry::activate() {
  m_assert(!activated_);
  //TotalTimer tmmm("loadTexture::activate");

  for (auto& itr : texture_images_) {
    auto& image = itr.second;
    GLuint tid;
    glGenTextures(1, &tid);
    glBindTexture(GL_TEXTURE_2D, tid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if (image.channels() == 3)
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width(), image.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, image.ptr());
    else
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.ptr());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    map_[itr.first] = tid;
  }
  activated_ = true;
}

void TextureRegistry::deactivate() {
  activated_ = false;
  for (auto& item: map_)
    glDeleteTextures(1, &item.second);
  map_.clear();
}


} // namespace render
