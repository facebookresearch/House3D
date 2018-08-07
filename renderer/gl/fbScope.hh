// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: fbScope.hh


#pragma once
#include "api.hh"

#include "lib/geometry.hh"
#include "lib/debugutils.hh"
#include "lib/mat.h"
#include "lib/strutils.hh"
#include "lib/imgproc.hh"

namespace render {

class Framebuffer {
  public:
    explicit Framebuffer(Geometry win_size): win_size_{win_size} {
      if (glGenFramebuffers == nullptr)
        error_exit("Pointer to glGenFramebuffers wasn't setup properly!");

      glGenFramebuffers(1, &fbo);
      glGenRenderbuffers(2, rbo);
      glBindRenderbuffer(GL_RENDERBUFFER, rbo[0]);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, win_size_.w, win_size_.h);

      glBindRenderbuffer(GL_RENDERBUFFER, rbo[1]);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, win_size_.w, win_size_.h);

      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo[0]);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo[1]);

      glBindRenderbuffer(GL_RENDERBUFFER, 0);

      GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if (status != GL_FRAMEBUFFER_COMPLETE)
        error_exit(
          ssprintf("ERROR::FRAMEBUFFER: Framebuffer is not complete! ErrorCode=%d\n", status));
    }

    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, fbo); }

    void unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    Matuc capture() const {
      Matuc ret{win_size_.h, win_size_.w, 4};
      glReadBuffer(GL_COLOR_ATTACHMENT0);
      glReadPixels(0, 0, win_size_.w, win_size_.h,
          GL_RGBA, GL_UNSIGNED_BYTE, ret.ptr());
      // opengl returns a vertical-flipped image.


      Matuc ret3{win_size_.h, win_size_.w, 3};
      for (int i = 0; i < ret3.height(); ++i) {
        unsigned char* oldptr = ret.ptr(i);
        unsigned char* newptr = ret3.ptr(win_size_.h - 1 - i);
        for (int j = 0; j < ret3.width(); ++j) {
          *(newptr++) = *(oldptr++);
          *(newptr++) = *(oldptr++);
          *(newptr++) = *(oldptr++);
          oldptr ++;
        }
      }
      return ret3;
    }

    ~Framebuffer() {
      glDeleteFramebuffers(1, &fbo);
      glDeleteRenderbuffers(2, rbo);
    }

  protected:
    GLuint fbo, rbo[2];
    Geometry win_size_;
};


class FramebufferScope {
  public:
    explicit FramebufferScope(const Framebuffer& fb) :
      fb_{fb} { fb.bind(); }

    Matuc capture() const { return fb_.capture(); }

    ~FramebufferScope() { fb_.unbind(); }

  private:
    const Framebuffer& fb_;
};

}

