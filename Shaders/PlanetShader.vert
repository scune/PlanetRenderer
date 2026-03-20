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



// Converts a random uint to a random float in [0, 1).
// https://github.com/rust-random/rand/blob/7aa25d577e2df84a5156f824077bb7f6bdf28d97/src/distributions/float.rs#L111-L117
float UintToUnitFloat(const uint i)
{
  // 32 - 8 = 24 (exponent), 1.0 / (1 << 24) = 5.9604644775390625e-08
  // top bits are usually more random.
  return (i >> 8) * 5.9604644775390625e-08;
}

uvec3 Pcg3d(uvec3 v)
{
  v = v * 1664525u + 1013904223u;
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  v ^= v >> 16u;
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  return v;
}

uint Seed(uvec3 p)
{
  return 19u * p.x + 47u * p.y + 101u * p.z + 131u;
}

float Hash(uvec3 v)
{
  v = Pcg3d(v);
  return UintToUnitFloat(Seed(v));
}

void main()
{
  outPos = inPos.xyz;
  float scale;
  if (inNormal.z > 0.6f)
  {
    scale = 100.f;
  }
  else
  {
    scale = 10.f;
  }
  uvec3 gridPos = uvec3(normalize(inPos.xyz) * scale + scale);
  outNormal = vec4(inNormal.xyz, Hash(gridPos) * 3.1415f);
  //debugPrintfEXT("Hash: %f", Hash(gridPos));
  //outNormal = inNormal;
  outVertexID = gl_VertexIndex;
  gl_Position = camMatrix * vec4(inPos.xyz, 1.f);
}
