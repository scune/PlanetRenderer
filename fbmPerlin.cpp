#include "fbmPerlin.hpp"

glm::vec2 CubeProj(const glm::vec3& pos, uint32_t& planeID, uint32_t& extent3,
                   glm::vec3& cubePos) noexcept
{
  const glm::vec3 absP = abs(pos);
  glm::vec3 uv = pos;

  if (absP.x > absP.y && absP.x > absP.z) // x
  {
    planeID = (uv.x > 0.f) ? 0 : 2;
    extent3 = 0x1A2;
    uv.x = uv.y / absP.x;
    uv.y = uv.z / absP.x;
    cubePos = pos / absP.x;
  }
  else if (absP.y > absP.z) // y
  {
    planeID = (uv.y > 0.f) ? 1 : 3;
    extent3 = 0x161;
    uv.x = uv.x / absP.y;
    uv.y = uv.z / absP.y;
    cubePos = pos / absP.y;
  }
  else // z
  {
    planeID = (uv.z > 0.f) ? 4 : 5;
    extent3 = 0xD1;
    uv.x /= absP.z;
    uv.y /= absP.z;
    cubePos = pos / absP.z;
  }
  cubePos = glm::clamp(cubePos * 0.5f + 0.5f, 0.f, 1.f);
  uv.x = glm::clamp(uv.x * 0.5f + 0.5f, 0.f, 1.f);
  uv.y = glm::clamp(uv.y * 0.5f + 0.5f, 0.f, 1.f);
  return glm::vec2(uv);
}

glm::uvec2 Pcg2D(glm::uvec2 v)
{
  v = v * 1664525u + 1013904223u;
  v.x += v.y * 1664525u;
  v.y += v.x * 1664525u;
  v = v ^ (v >> 16u);
  v.x += v.y * 1664525u;
  v.y += v.x * 1664525u;
  v = v ^ (v >> 16u);
  return v;
}

glm::vec2 Hash2D(glm::uvec2 u)
{
  u = Pcg2D(u);
  return glm::vec2(u >> 8u) * 5.9604644775390625e-08f * 2.0f - 1.0f;
}

glm::uvec3 Pcg3D(glm::uvec3 v)
{
  v = v * 1664525u + 1013904223u;
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  v ^= (v >> 16u);
  v.x += v.y * v.z;
  v.y += v.z * v.x;
  v.z += v.x * v.y;
  return v;
}

glm::vec3 Hash3D(glm::uvec3 u)
{
  u = Pcg3D(u);
  return glm::vec3(u >> 8u) * 5.9604644775390625e-08f * 2.0f - 1.0f;
}

glm::uvec3 Extract(const uint32_t extent3, const int offset)
{
  return glm::uvec3(glm::bitfieldExtract(extent3, offset + 0, 1),
                    glm::bitfieldExtract(extent3, offset + 1, 1),
                    glm::bitfieldExtract(extent3, offset + 2, 1));
}

glm::uvec3 Extract1(const uint32_t extent3) { return Extract(extent3, 0); }
glm::uvec3 Extract2(const uint32_t extent3) { return Extract(extent3, 3); }
glm::uvec3 Extract3(const uint32_t extent3) { return Extract(extent3, 6); }

glm::vec2 HashProjectToFace(glm::vec3 hash, uint32_t planeID)
{
  switch (planeID)
  {
  case 0:
  case 2:
    return glm::vec2(hash.y, hash.z);
  case 1:
  case 3:
    return glm::vec2(hash.x, hash.z);
  case 4:
  case 5:
  default:
    return glm::vec2(hash.x, hash.y);
  }
}

glm::vec3 PerlinNoise(const glm::uvec2 uv, const glm::vec2 uvFract,
                      const glm::uvec3 cubePos, const glm::uvec3 extent,
                      const uint32_t extent3, const uint32_t planeID)
{
  glm::vec2 ga = Hash2D(uv + glm::uvec2(0, 0));
  glm::vec2 gb = Hash2D(uv + glm::uvec2(1, 0));
  glm::vec2 gc = Hash2D(uv + glm::uvec2(0, 1));
  glm::vec2 gd = Hash2D(uv + glm::uvec2(1, 1));

  if (uv.x == extent.x) // Min x
  {
    glm::vec3 ga3 = Hash3D(cubePos);
    glm::vec3 gc3 = Hash3D(cubePos + Extract2(extent3));
    ga = HashProjectToFace(ga3, planeID);
    gc = HashProjectToFace(gc3, planeID);
  }
  else if (uv.x == extent.y - 1u) // Max x
  {
    glm::vec3 gb3 = Hash3D(cubePos + Extract1(extent3));
    glm::vec3 gd3 = Hash3D(cubePos + Extract3(extent3));
    gb = HashProjectToFace(gb3, planeID);
    gd = HashProjectToFace(gd3, planeID);
  }

  if (uv.y == 0u) // Min y
  {
    glm::vec3 ga3 = Hash3D(cubePos);
    glm::vec3 gb3 = Hash3D(cubePos + Extract1(extent3));
    ga = HashProjectToFace(ga3, planeID);
    gb = HashProjectToFace(gb3, planeID);
  }
  else if (uv.y == extent.z - 1u) // Max y
  {
    glm::vec3 gc3 = Hash3D(cubePos + Extract2(extent3));
    glm::vec3 gd3 = Hash3D(cubePos + Extract3(extent3));
    gc = HashProjectToFace(gc3, planeID);
    gd = HashProjectToFace(gd3, planeID);
  }

  float va = glm::dot(ga, uvFract - glm::vec2(0.0f, 0.0f));
  float vb = glm::dot(gb, uvFract - glm::vec2(1.0f, 0.0f));
  float vc = glm::dot(gc, uvFract - glm::vec2(0.0f, 1.0f));
  float vd = glm::dot(gd, uvFract - glm::vec2(1.0f, 1.0f));

  glm::vec2 u = uvFract * uvFract * uvFract *
                (uvFract * (uvFract * 6.0f - 15.0f) + 10.0f);

  glm::vec2 du =
      30.0f * uvFract * uvFract * (uvFract * (uvFract - 2.0f) + 1.0f);

  float res_value =
      va + u.x * (vb - va) + u.y * (vc - va) + u.x * u.y * (va - vb - vc + vd);

  glm::vec2 res_deriv = ga + u.x * (gb - ga) + u.y * (gc - ga) +
                        u.x * u.y * (ga - gb - gc + gd) +
                        du * (glm::vec2(u.y, u.x) * (va - vb - vc + vd) +
                              glm::vec2(vb - va, vc - va));

  return glm::vec3(res_value, res_deriv.x, res_deriv.y);
}

float FbmPerlin(glm::vec2 p, glm::vec3 cubePosF, const uint32_t planeID,
                const uint32_t extent3, const uint32_t fbmFrequency,
                const float amplitudePercent, const uint8_t iterations) noexcept
{
  p.x += (float)planeID;
  p *= fbmFrequency;

  glm::uvec3 extent = glm::uvec3(planeID * fbmFrequency,
                                 (planeID + 1) * fbmFrequency, fbmFrequency);
  glm::vec2 uvFract = glm::fract(p);
  glm::uvec2 uv = glm::uvec2(p);

  glm::vec3 an = glm::vec3(0.0f);
  float b = 1.0f;
  glm::vec2 d = glm::vec2(0.0f);

  cubePosF *= fbmFrequency;
  glm::vec3 cubeFract = glm::fract(cubePosF);
  glm::uvec3 cubePos = glm::uvec3(cubePosF);

  for (uint8_t i = 0; i < iterations; i++)
  {
    glm::vec3 n = PerlinNoise(uv, uvFract, cubePos, extent, extent3, planeID);

    glm::vec2 deriv = glm::vec2(n.y, n.z) * amplitudePercent;

    float erosion = 1.0f + glm::dot(d, d);
    an.y += deriv.x / erosion;
    an.z += deriv.y / erosion;

    d += deriv;

    float erosion2 = 1.0f + glm::dot(d, d);
    an.x += b * n.x / erosion2;

    b *= 0.5f;
    extent <<= 1;

    uv <<= 1;
    uvFract *= 2.0f;
    uv += glm::uvec2(uvFract);
    uvFract = glm::fract(uvFract);

    cubePos <<= 1;
    cubeFract *= 2.0f;
    cubePos += glm::uvec3(cubeFract);
    cubeFract = glm::fract(cubeFract);
  }
  return an.x;
}
