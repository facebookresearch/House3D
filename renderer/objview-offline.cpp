// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: objview-offline.cpp


#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>

#include "model/scene.hh"
#include "model/mesh.hh"
#include "gl/glContext.hh"
#include "gl/utils.hh"
#include "gl/camera.hh"
#include "gl/fbScope.hh"
#include "lib/timer.hh"
#include "lib/imgproc.hh"

#include "model/shader.hh"

using namespace render;
using namespace std;

int main(int argc, char* argv[]) {
  TotalTimerGlobalGuard TGGG;
  if (argc < 2)
    return 1;
  bool benchmark = argc == 3;
  // argv[1]: obj filename
  // argv[2]: if exists, run benchmarks instead of write output

  Geometry geo{800, 600};
  unique_ptr<GLContext> ctx{createHeadlessContext(geo, 0)};
  ctx->printInfo();

  tryEnableGLDebug();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  SimpleObjScene scene(argv[1]);
  auto& shader = *scene.get_shader();
  shader.use();

  auto range = scene.get_range();
  auto mid = scene.get_min() + range * 0.5f;
  mid.z += glm::compMax(range);

  Camera camera(mid);

  Speedometer spd;
  Framebuffer fb{geo};
  while (true) {
    spd.update();
    FramebufferScope fbs{fb};
    camera.shift(Camera::Movement::FORWARD, 0.1);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.setMat4("projection", camera.getCameraMatrix(geo));
    shader.setVec3("eye", camera.pos);

    // render the loaded model
    scene.draw();
    auto mat = fb.capture();
    if (not benchmark) {
      write_rgb("out.jpg", mat);
      break;
    }
  }
}
