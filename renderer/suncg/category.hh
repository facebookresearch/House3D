// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: category.hh

#pragma once
#include <csv.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <iostream>

#include "lib/debugutils.hh"
#include "tiny_obj_loader.h"

namespace render {
class ModelCategory final {
  public:
    explicit ModelCategory(std::string fname) {
      reader_.reset(new io::CSVReader<3>{fname});
        read_csv();
    }

    // remove the shapes matching coarse grained categories
    void filter_category(
        std::vector<tinyobj::shape_t>& shapes,
        const std::unordered_set<std::string>& categories) {
      shapes.erase(
          std::remove_if(shapes.begin(),
            shapes.end(),
            // remove person from the scene
            [&](const tinyobj::shape_t& shape) {
              std::string name = shape.name;
              static std::string prefix = "Model#";
              if (name.substr(0, prefix.size()) != prefix)
                return false;
              name = name.substr(prefix.size());
              auto itr = coarse_grained_class_.find(name);
              if (itr != coarse_grained_class_.end())
                if (categories.count(itr->second)) {
                  print_debug("Removing %s of class %s\n", shape.name.c_str(), itr->second.c_str());
                  return true;
                }
            return false;
            }), shapes.end()
          );
    }

    std::string get_coarse_grained_class(std::string model_id) {
      auto itr = coarse_grained_class_.find(model_id);
      if (itr == coarse_grained_class_.end()) {
        print_debug("Cannot find model %s\n", model_id.c_str());
        return "";
      }
      return itr->second;
    }

    std::string get_fine_grained_class(std::string model_id) {
      auto itr = fine_grained_class_.find(model_id);
      if (itr == fine_grained_class_.end()) {
        print_debug("Cannot find model %s\n", model_id.c_str());
        return "";
      }
      return itr->second;
    }

  private:
    std::unordered_map<std::string, std::string> coarse_grained_class_;
    std::unordered_map<std::string, std::string> fine_grained_class_;
    std::unique_ptr<io::CSVReader<3>> reader_;

    void read_csv() {
      reader_->read_header(io::ignore_extra_column, "model_id", "fine_grained_class", "coarse_grained_class");
      std::string modelId, fineClass, coarseClass;
      while (reader_->read_row(modelId, fineClass, coarseClass)) {
        fine_grained_class_.emplace(modelId, fineClass);
        coarse_grained_class_.emplace(modelId, coarseClass);
      }
    }

};

}
