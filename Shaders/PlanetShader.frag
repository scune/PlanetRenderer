#version 460
#extension GL_EXT_debug_printf : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_GOOGLE_include_directive : enable

#include "PlanetCommon.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in flat uint inVertexID;

layout(binding = 1) uniform sampler2D texture1;
layout(binding = 2) uniform sampler2D texture2;

layout(location = 0) out vec4 Swapchain;



#define LIGHT vec3(-0.2f, -0.3f, 0.6f)

float Light()
{
  return max(0.3f, dot(inNormal.xyz, LIGHT));
}

uint PcgHash(uint seed)
{
  uint state = seed * 747796405u + 2891336453u;
  uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	seed = (word >> 22u) ^ word;
  return seed;
}

vec3 UintToColor(uint i)
{
  return vec3(i & 0x7FF,
              (i >> 11) & 0x7FF,
              i >> 22) / float(0x7FF);
}

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

vec3 Hash3D(uvec3 u)
{
  u = Pcg3d(u);
  vec3 v;
  v.x = UintToUnitFloat(u.x);
  v.y = UintToUnitFloat(u.y);
  v.z = UintToUnitFloat(u.z);
  return v;
}

vec2 Rotate(vec2 v, float angle)
{
  float s = sin(angle);
  float c = cos(angle);
  return vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

uvec2 Seed3D2D(uvec3 p)
{
  //return uvec2(dot(p.xy, uvec2(19, 47)),
  //             dot(uvec2(p.z, 1), uvec2(101, 131)));
  return uvec2(19u * p.x + 47u * p.y,
               101u * p.z + 131u);
}

vec2 Hash3D2D(uvec3 u)
{
  u = Pcg3d(u);
  u.xy = Seed3D2D(u);
  return vec2(UintToUnitFloat(u.x),
              UintToUnitFloat(u.y));
}

uint Seed3D1D(uvec3 p)
{
  //return dot(uvec4(p, 1u), uvec4(19u, 47u, 101u, 131u));
  return 19u * p.x + 47u * p.y + 101u * p.z + 131u;
}

float Hash3D1D(uvec3 v)
{
  return UintToUnitFloat(Seed3D1D(Pcg3d(v)));
}

vec3 FastTangent(vec3 v)
{
  const float negY = -v.y;
  return (v.y < 0.9f) ? vec3(negY, 0.f, v.x) : // cross(v, vec3(0.f, 1.f, 0.f))
                        vec3(0.f, v.z, negY); // cross(v, vec3(1.f, 0.f, 0.f))
}

vec3 Voronoi(const vec3 pos, const float cellMin, const vec3 tangent, const vec3 bitangent,
             out vec3 outF1, out vec3 outF2)
{
  vec3 secClosestCell = pos - tangent - bitangent;
  vec3 closestCell = floor(secClosestCell) + Hash3D(uvec3(secClosestCell + cellMin));
  float minDistToCell = distance(pos, closestCell);
  float secMinDistToCell = minDistToCell;
  [[unroll]]
  for(float x = -1.f; x <= 1.f; x++)
  [[unroll]]
  for(float y = -1.f; y <= 1.f; y++)
  {
    if (x == -1.f && y == -1.f)
      continue;

    vec3 cell = pos + tangent * x + bitangent * y;
    vec3 rndCellPos = floor(cell) + Hash3D(uvec3(cell + cellMin));
    float dist = distance(pos, rndCellPos);
    if (dist < minDistToCell)
    {
      minDistToCell = dist;
      secClosestCell = closestCell;
      closestCell = rndCellPos;
    }
    else if (dist < secMinDistToCell)
    {
      secClosestCell = rndCellPos;
      secMinDistToCell = dist;
    }
  }
  outF1 = closestCell;
  outF2 = secClosestCell;
  return vec3(Hash3D2D(uvec3(closestCell + cellMin)), minDistToCell);
  /*return vec3(Hash3D1D(uvec3(closestCell + cellMin)),
              Hash3D1D(uvec3(secClosestCell + cellMin)),
              minDistToCell);*/
}

vec3 TerrainColor()
{
  const float slope = inNormal.z;
  const float scale = (slope > 0.6f) ? 100.f : 30.f;

  vec3 tangent = FastTangent(inNormal.xyz);
  vec3 bitangent = cross(inNormal.xyz, tangent);

  vec3 gridPos = floor(inPos / 10000.f * scale);
  vec3 f1;
  vec3 f2;
  vec3 voronoiHash = Voronoi(gridPos, scale, tangent, bitangent, f1, f2);
  return vec3(voronoiHash.x);

  float d = dot(0.5f*(f1+f2),normalize(f2-f1));
  return vec3(d);

  float hash = voronoiHash.x;

  vec2 uv;
  uv.x = dot(fract(gridPos), tangent);
  uv.y = dot(fract(gridPos), bitangent);
  /*
  uv = Rotate(uv, hash * 3.1415f * 2.f); // Random rotation
  uv *= hash * 0.7f + 0.1f; // Random scaling
  uv += hash * 0.5f; // Random offset
*/
  // uv += voronoiHash.xy;

  vec3 color;
  if (slope > 0.6f)
    color = texture(texture2, uv).xyz;
  else
    color = texture(texture1, uv).xyz;

  return color;

  float blendFactor = (voronoiHash.z > 0.8f) ? 0.5f : 0.f;
  //return vec3(blendFactor);
  if (blendFactor > 0.2f)
  {
    hash = voronoiHash.y;

    uv.x = dot(fract(gridPos), tangent);
    uv.y = dot(fract(gridPos), bitangent);

    uv = Rotate(uv, hash * 3.1415f * 2.f); // Random rotation
    uv *= hash * 0.7f + 0.1f; // Random scaling
    uv += hash * 0.5f; // Random offset

    vec3 color2;
    if (slope > 0.6f)
      color2 = texture(texture2, uv).xyz;
    else
      color2 = texture(texture1, uv).xyz;

    color = mix(color, color2, blendFactor);
  }
  return color;
}

void main()
{
  // Swapchain = vec4(Light());
  //Swapchain = vec4(TerrainColor(), 1.f);
  //Swapchain = vec4(inNormal.w);
  Swapchain = vec4(floor(inPos.xyz * 0.00309978f) / 30.f, 1.f);
  //Swapchain = vec4(UintToColor(PcgHash(inVertexID)) + vec3(0.f, 0.f, 0.5f), 1.f);
  // Swapchain = vec4(inNormal, 1.f);
  // Swapchain = vec4(TerrainColor(inPos) * Light(), 1.f);
}
