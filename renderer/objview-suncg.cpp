// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: objview-suncg.cpp


#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>

#include "gl/glContext.hh"
#include "model/mesh.hh"
#include "gl/utils.hh"
#include "gl/camera.hh"
#include "lib/timer.hh"

#include "suncg/scene.hh"
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
  if (argc != 4)
    return 1;
  GLFWContext ctx{Geometry{800, 600}, true};
  ctx.printInfo();
  auto& window = ctx.get_window();


  tryEnableGLDebug();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  SUNCGScene scene(argv[1], argv[2], argv[3]);
  auto& shader = *scene.get_shader();
  shader.use();

  auto range = scene.get_range();
  cout << "Range:" << range << endl;
  auto mid = scene.get_min() + range * 0.5f;
  mid.z += glm::compMax(range);

  Camera camera(mid);
  CameraController ctrl{window, camera};
  //scene.set_mode(SUNCGScene::RenderMode::DEPTH);

  controlledMainLoop(ctrl, [&]() {
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // view/projection transformations
      glm::mat4 projection = glm::perspective(glm::radians(60.f), 800.0f / 600.0f, 0.1f, 100.0f);
      glm::mat4 view = camera.getView();
      shader.setMat4("projection", projection * view);
      shader.setVec3("eye", camera.pos);

      // render the loaded model
      scene.draw();
  });
}
