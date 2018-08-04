// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: glContext.hh

#pragma once

#define INCLUDE_GL_CONTEXT_HEADERS
#include "api.hh"
#undef INCLUDE_GL_CONTEXT_HEADERS

#include "lib/geometry.hh"

namespace render {

class GLContext {
  public:
    GLContext(Geometry win_size): win_size_{win_size} {}
    virtual ~GLContext() {}

    virtual void printInfo();

  protected:
    void init();
    Geometry win_size_;
};


class GLFWContext : public GLContext {
  public:
    GLFWContext(Geometry win_size, bool core=true);
    ~GLFWContext();

    GLFWwindow& get_window() { return *window_; }

  protected:
    GLFWwindow* window_;
};


#ifdef __linux__
// Context for EGL (server-side OpenGL on some supported GPUs)
class EGLContext : public GLContext {
  public:
    EGLContext(Geometry win_size, int device=0);
    ~EGLContext();

  protected:
    EGLDisplay eglDpy_;
    ::EGLContext eglCtx_;
};

// Context for GLX (OpenGL to X11)
class GLXHeadlessContext : public GLContext {
  public:
    GLXHeadlessContext(Geometry win_size);
    ~GLXHeadlessContext();

  protected:
    Display* dpy_;
    GLXContext context_;
    GLXPbuffer pbuffer_;
};
#endif

#ifdef __APPLE__
// Apple use CGL (Core OpenGL to initialize context)
class CGLHeadlessContext : public GLContext {
  public:
    CGLHeadlessContext(Geometry win_size);
    ~CGLHeadlessContext();

  protected:
    CGLContextObj context_;
};
#endif

// Create a headless context, either EGLContext, GLXHeadlessContext, or CGLContext,
// depending on OS, and DISPLAY environment variable
// The caller owns the pointer.
inline GLContext* createHeadlessContext(Geometry win_size, int device=0) {
#ifdef __APPLE__
  m_assert(device == 0);
  return new CGLHeadlessContext{win_size};
#endif
#ifdef __linux__
  // prefer GLX (better compatibility with GPUs) when device=0
  if (device == 0 and std::getenv("DISPLAY") != nullptr)
      return new GLXHeadlessContext{win_size};
  return new EGLContext{win_size, device};
#endif
  error_exit("Neither Apple nor Linux!");
};

} // namespace render
