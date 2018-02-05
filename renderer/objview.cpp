// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: objview.cpp


#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/component_wise.hpp>

#include "gl/glContext.hh"
#include "model/scene.hh"
#include "model/mesh.hh"
#include "gl/utils.hh"
#include "gl/camera.hh"

#include "lib/timer.hh"
#include "model/shader.hh"

using namespace render;
using namespace std;

void controlledMainLoop(CameraController& ctrl, function<void(void)> render_func) {
  Speedometer fps;
  auto& window = ctrl.getWindow();
  while (!glfwWindowShouldClose(&window)) {
    // Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
    glfwPollEvents();
    ctrl.moveCamera();
    fps.update();

    render_func();

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(&window);
  }
}


int main(int argc, char* argv[]) {
  if (argc != 2)
    return 1;
  Geometry geo{800, 600};
  GLFWContext ctx{geo, true};
  ctx.printInfo();
  auto& window = ctx.get_window();

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
  CameraController ctrl{window, camera};

  controlledMainLoop(ctrl, [&]() {
      glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      shader.setMat4("projection", camera.getCameraMatrix(geo));
      shader.setVec3("eye", camera.pos);

      // render the loaded model
      scene.draw();
  });
}
