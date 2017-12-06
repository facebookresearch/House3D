// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: utils.cc

#include "utils.hh"
#include "lib/debugutils.hh"

#include <vector>

namespace render {

bool tryEnableGLDebug() {
#ifdef DEBUG
#ifdef GL_KHR_debug
  if (checkExtension("GL_ARB_debug_output")) {
    glEnable(GL_DEBUG_OUTPUT);
    return true;
  }
#else
  print_debug("KHR_debug extension unavailable!");
#endif
#endif
  return false;
}

void printGLDebugMsg(GLuint num) {
#ifdef GL_KHR_debug
  GLint maxMsgLen = 0;
  glGetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &maxMsgLen);

  std::vector<GLchar> msgData(num * maxMsgLen);
  std::vector<GLenum> sources(num);
  std::vector<GLenum> types(num);
  std::vector<GLenum> severities(num);
  std::vector<GLuint> ids(num);
  std::vector<GLsizei> lengths(num);

  GLuint num_fetched = glGetDebugMessageLog(num,
      msgData.size(),
      &sources[0], &types[0],
      &ids[0], &severities[0],
      &lengths[0], &msgData[0]);

  lengths.resize(num_fetched);
  GLchar* currPos = msgData.data();
  for(auto& l : lengths) {
    print_debug("%s\n", std::string(currPos, currPos + l - 1).c_str());
    currPos = currPos + l;
  }
#else
  print_debug("KHR_debug extension unavailable!");
#endif
}

bool checkExtension(const std::string& ext) {
  GLint n;
  glGetIntegerv(GL_NUM_EXTENSIONS, &n);
  for (int i = 0; i < n; ++i) {
    const GLubyte* s = glGetStringi(GL_EXTENSIONS, i);
    std::string sname{reinterpret_cast<const char*>(s)};
    if (ext.compare(sname) == 0)
      return true;
  }
  return false;
}


} // namespace render
