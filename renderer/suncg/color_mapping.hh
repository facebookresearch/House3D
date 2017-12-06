// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: color_mapping.hh

#pragma once

#include <csv.h>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>

#include "lib/debugutils.hh"

namespace render {

// A case-insensitive map, from a string to rgb
class ColorMappingReader final {
  public:
    // fname: a comma-separated csv with column:
    // name,r,g,b. r,g,b are integers in range [0,255]
    explicit ColorMappingReader(std::string fname) {
      reader_.reset(new io::CSVReader<4>{fname});

      reader_->read_header(io::ignore_extra_column, "name", "r", "g", "b");
      std::string name;
      unsigned int r, g, b;
      while (reader_->read_row(name, r, g, b)) {
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        colormap_.emplace(name, glm::vec3{r/255.0, g/255.0, b/255.0});
      }
    }

    glm::vec3 get_color(std::string klass) {
      std::transform(klass.begin(), klass.end(), klass.begin(), ::tolower);
      auto itr = colormap_.find(klass);
      if (itr == colormap_.end()) {
        print_debug("Couldn't find color for class %s\n", klass.c_str());
        return {0,0,0};
      }
      return itr->second;
    }

    glm::vec3 get_background_color() {
      return colormap_["other"];
    }

    int size() const { return colormap_.size(); }

  private:
    std::unordered_map<std::string, glm::vec3> colormap_;
    std::unique_ptr<io::CSVReader<4>> reader_;
};

}
