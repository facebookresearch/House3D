// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: test-rectangle.cpp


#include "gl/glContext.hh"
#include "gl/fbScope.hh"

#include "rectangle/scene.hh"
#include "lib/imgproc.hh"

using namespace render;
using namespace std;

// Is called whenever a key is pressed/released via GLFW
void GLFW_key_callback(GLFWwindow* window, int key,
    int /*scancode*/, int action, int /*mode*/) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    error_exit("Unknown Command");
    return 1;
  }
  string cmd = argv[1];
  Geometry geo{800, 600};

  if (cmd == "headless") {
#ifdef __linux__
    render::GLXHeadlessContext ctx{geo};
#else
    render::CGLHeadlessContext ctx{geo};
#endif
    ctx.printInfo();

    Framebuffer fb{geo};
    FramebufferScope fbs{fb};
    RectangleScene sc;
    sc.draw();
    auto mat = fb.capture();
    write_rgb("out.jpg", mat);
#ifdef __linux__
  } else if (cmd == "egl") {
    render::EGLContext ctx{geo};
    ctx.printInfo();

    Framebuffer fb{geo};
    FramebufferScope fbs{fb};
    RectangleScene sc;
    sc.draw();
    auto mat = fb.capture();
    write_rgb("out.jpg", mat);
#endif
  } else if (cmd == "glfw") {
    render::GLFWContext ctx{Geometry{800, 600}, true};
    ctx.printInfo();

    Framebuffer fb{geo};
    // Set the required callback functions
    glfwSetKeyCallback(&ctx.get_window(), GLFW_key_callback);

    auto& window = ctx.get_window();
    RectangleScene sc;

    while (!glfwWindowShouldClose(&window)) {
      // Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
      glfwPollEvents();
      sc.draw();
      // Swap the screen buffers
      glfwSwapBuffers(&window);
    }
  } else {
    error_exit("Unknown Command");
  }
  return 0;
}
