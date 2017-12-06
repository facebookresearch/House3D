// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: shader.cc

#include "shader.hh"

namespace render {

const char* BasicShader::vShader = R"xxx(
#version 330 core
layout (location = 0) in vec3 posIn;
layout (location = 1) in vec3 normalIn;
layout (location = 2) in vec2 texcoordIn;

out vec3 pos;
out vec3 normal;
out vec2 texcoord;

uniform mat4 projection;

void main()
{
    texcoord = texcoordIn;
    normal = normalize(normalIn);
    pos = posIn;
    gl_Position = projection * vec4(posIn, 1.0f);
}
)xxx";

const char* BasicShader::fShader = R"xxx(
#version 330 core

in vec3 pos;
in vec3 normal;
in vec2 texcoord;
out vec4 fragcolor;

uniform uint mode;
// 0: light + texture
// 1: light
uniform vec3 Kd;
uniform vec3 Ka;
uniform vec3 eye;
uniform float dissolve;
uniform sampler2D texture_diffuse;

float max3(vec3 v) {
  return max(max(v.x, v.y), v.z);
}

void main()
{
    vec3 color;
    float alpha = dissolve;
    switch(mode) {
      case 0u:
        vec4 texcolor = texture(texture_diffuse, texcoord);
        if (max3(Kd) == 0.f)
          // if we have texture, but Kd is black, ignore Kd
          color = texcolor.xyz;
        else
          // otherwise, multiply them (is this the right thing to do?)
          color = Kd * texcolor.xyz;
        alpha = min(texcolor.w, alpha);
        break;
      case 1u:
        color = Kd;
        break;
    }
    vec3 in_vec = normalize(eye - pos);
    // have some diffuse color even when orthogonal
    float scale = max(dot(in_vec, normal), 0.3f);
    vec3 ambient = Ka * 0.1;
    color = color * scale + ambient;
    color = clamp(color, 0.f, 1.f);
    fragcolor = vec4(color, alpha);
}
)xxx";


BasicShader::BasicShader(): Shader{vShader, fShader} {
  Kd_loc = getUniformLocation("Kd");
  Ka_loc = getUniformLocation("Ka");
  mode_loc = getUniformLocation("mode");
  texture_loc = getUniformLocation("texture_diffuse");
  dissolve_loc = getUniformLocation("dissolve");
};

}
