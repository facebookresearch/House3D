// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: glContext.cc

#define INCLUDE_GL_CONTEXT_HEADERS
#include "api.hh"
#undef INCLUDE_GL_CONTEXT_HEADERS
#include "glContext.hh"
#include <iostream>

#ifdef __linux__
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "lib/debugutils.hh"
#include "lib/strutils.hh"

using namespace std;

namespace {

#ifdef __linux__
const EGLint EGLconfigAttribs[] = {
  EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
  EGL_BLUE_SIZE, 8,
  EGL_GREEN_SIZE, 8,
  EGL_RED_SIZE, 8,
  EGL_DEPTH_SIZE, 24,
  EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
  EGL_NONE
};

const EGLint EGLpbufferAttribs[] = {
  EGL_WIDTH, 9,
  EGL_HEIGHT, 9,
  EGL_NONE,
};

const int GLXcontextAttribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
    None
};


const int GLXpbufferAttribs[] = {
  GLX_PBUFFER_WIDTH,  9,
  GLX_PBUFFER_HEIGHT, 9,
  None
};

#endif

#ifdef __APPLE__
CGLPixelFormatAttribute CGLAttribs[4] = {
  kCGLPFAAccelerated,   // no software rendering
  kCGLPFAOpenGLProfile, // core profile with the version stated below
  (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core,
  (CGLPixelFormatAttribute) 0
};
#endif

}

namespace render {

void GLContext::init() {
  glViewport(0, 0, win_size_.w, win_size_.h);
}

void GLContext::printInfo() {
  m_assert(glGetString(GL_VERSION));
  cerr << "----------- OpenGL Context Info --------------" << endl;
  cerr << "GL Version: " << glGetString(GL_VERSION) << endl;
  cerr << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
  cerr << "Vendor: " << glGetString(GL_VENDOR) << endl;
  cerr << "Renderer: " << glGetString(GL_RENDERER) << endl;
  cerr << "----------------------------------------------" << endl;
}


GLFWContext::GLFWContext(Geometry win_size, bool core): GLContext{win_size} {
  glfwInit();
  // Set all the required options for GLFW
  if (core) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#ifdef DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
  }
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  // Create a GLFWwindow object that we can use for GLFW's functions
  window_ = glfwCreateWindow(win_size_.w, win_size_.h, "GLFW", nullptr, nullptr);
  if (window_ == nullptr)
    error_exit("Failed to create GLFW window!");
  glfwMakeContextCurrent(window_);

  this->init();
}

GLFWContext::~GLFWContext() {
  glfwTerminate();
}


#ifdef __linux__
// https://devblogs.nvidia.com/parallelforall/egl-eye-opengl-visualization-without-x-server/
EGLContext::EGLContext(Geometry win_size, int device): GLContext{win_size} {
  auto checkError = [](EGLBoolean succ) {
    EGLint err = eglGetError();
    if (err != EGL_SUCCESS)
      error_exit(ssprintf("EGL error: %d\n", err));
    if (!succ)
      error_exit("EGL failed!\n");
  };

  // 1. Initialize EGL
  {
    static const int MAX_DEVICES = 16;
    EGLDeviceEXT eglDevs[MAX_DEVICES];
    EGLint numDevices;
    PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT =
      (PFNEGLQUERYDEVICESEXTPROC) eglGetProcAddress("eglQueryDevicesEXT");
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
      (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (!eglQueryDevicesEXT or !eglGetPlatformDisplayEXT) {
      error_exit("Failed to get function pointer of eglQueryDevicesEXT/eglGetPlatformDisplayEXT! Maybe EGL extensions are unsupported.");
    }

    eglQueryDevicesEXT(MAX_DEVICES, eglDevs, &numDevices);
    cerr << "[EGL] Detected " << numDevices << " devices. Using device " << device << endl;
    m_assert(device < numDevices);
    eglDpy_ = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevs[device], 0);
  }

  EGLint major, minor;

  EGLBoolean succ = eglInitialize(eglDpy_, &major, &minor);
  if (!succ) {
    string dev = ssprintf("/dev/nvidia%d", device);
    int ret = open(dev.c_str(), O_RDONLY);
    if (ret == -1) {
      error_exit(ssprintf("Cannot access %s, failed to initialize EGL display! See README if you're inside docker.", dev.c_str()));
    }
    error_exit("Failed to initialize EGL display!");
  }
  checkError(succ);

  // 2. Select an appropriate configuration
  EGLint numConfigs;
  EGLConfig eglCfg;

  succ = eglChooseConfig(eglDpy_, EGLconfigAttribs, &eglCfg, 1, &numConfigs);
  checkError(succ);
  if (numConfigs != 1) {
    error_exit("Cannot create configs for EGL! You driver may not support EGL.");
  }

  // 3. Create a surface
  /*
   *EGLSurface eglSurf = eglCreatePbufferSurface(eglDpy_, eglCfg, EGLpbufferAttribs);
   *checkError(succ);
   */
  EGLSurface eglSurf = EGL_NO_SURFACE;

  // 4. Bind the API
  succ = eglBindAPI(EGL_OPENGL_API);
  checkError(succ);

  // 5. Create a context and make it current
  ::EGLContext eglCtx = eglCreateContext(eglDpy_, eglCfg, (::EGLContext)0, NULL);
  checkError(succ);
  succ = eglMakeCurrent(eglDpy_, eglSurf, eglSurf, eglCtx);
  if (!succ)
    error_exit("Failed to make EGL context current!");
  checkError(succ);

  this->init();
}

EGLContext::~EGLContext() {
  // 6. Terminate EGL when finished
  eglTerminate(eglDpy_);
}

GLXHeadlessContext::GLXHeadlessContext(Geometry win_size): GLContext{win_size} {
  dpy_ = XOpenDisplay(NULL);
  if (dpy_ == nullptr)
    error_exit("Cannot connect to DISPLAY!");

  static int visualAttribs[] = { None };
  int numberOfFramebufferConfigurations = 0;
  GLXFBConfig* fbc = glXChooseFBConfig(dpy_,
      DefaultScreen(dpy_), visualAttribs, &numberOfFramebufferConfigurations);

  // setup function pointers
  typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
  static glXCreateContextAttribsARBProc glXCreateContextAttribsARB = NULL;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

  GLXContext openGLContext = glXCreateContextAttribsARB(dpy_, fbc[0], 0, True, GLXcontextAttribs);

  GLXPbuffer pbuffer = glXCreatePbuffer(dpy_, fbc[0], GLXpbufferAttribs);

  XFree(fbc);
  XSync(dpy_, False);
  if (!glXMakeContextCurrent(dpy_, pbuffer, pbuffer, openGLContext))
    error_exit("Cannot create GLX context!");

  this->init();
}

GLXHeadlessContext::~GLXHeadlessContext() {
  XCloseDisplay(dpy_);
}

#endif

#ifdef __APPLE__
CGLHeadlessContext::CGLHeadlessContext(Geometry win_size): GLContext{win_size} {
  auto checkError = [](CGLError err) {
    if (err == CGLError::kCGLNoError)
      return;
    error_exit(ssprintf("Error %d when creating CGL Context", err));
  };
  CGLPixelFormatObj pix;
  GLint num; // stores the number of possible pixel formats
  CGLError err = CGLChoosePixelFormat(CGLAttribs, &pix, &num);
  checkError(err);
  err = CGLCreateContext(pix, nullptr, &context_); // second parameter can be another context for object sharing
  checkError(err);
  CGLDestroyPixelFormat(pix);
  err = CGLSetCurrentContext(context_);
  checkError(err);
  this->init();
}

CGLHeadlessContext::~CGLHeadlessContext() {
  CGLSetCurrentContext(nullptr);
  CGLDestroyContext(context_);
}
#endif

} // namespace render
