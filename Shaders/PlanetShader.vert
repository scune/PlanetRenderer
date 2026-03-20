#version 460
#extension GL_EXT_debug_printf : require
#extension GL_GOOGLE_include_directive : enable

#include "PlanetCommon.glsl"

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inNormal;

layout(binding = 0) uniform GlobalUpdate
{
  mat4 camMatrix;
};

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out flat uint outVertexID;



void main()
{
  outPos = inPos.xyz;
  outNormal = inNormal;
  outVertexID = gl_VertexIndex;
  gl_Position = camMatrix * vec4(inPos.xyz, 1.f);
}
