#ifndef DISPLACEMENT_GLSL
#define DISPLACEMENT_GLSL

// Accepts a signed normalized pos
// Returns UV in interval [0.f, 1.f]
// and ID of the plane from the cube
vec2 CubeProj(const vec3 pos, out uint planeID, out uint extent3, out vec3 cubePos)
{
  const vec3 absP = abs(pos);
  vec3 uv = pos;

  if (absP.x > absP.y && absP.x > absP.z) // x
  {
    planeID = (uv.x > 0.f) ? 0 : 2;
    extent3 = 0x1A2;
    uv.xy = uv.yz / absP.x;
    cubePos = pos / absP.x;
  }
  else if (absP.y > absP.z) // y
  {
    planeID = (uv.y > 0.f) ? 1 : 3;
    extent3 = 0x161;
    uv.xy = uv.xz / absP.y;
    cubePos = pos / absP.y;
  }
  else // z
  {
    planeID = (uv.z > 0.f) ? 4 : 5;
    extent3 = 0xD1;
    uv.xy /= absP.z;
    cubePos = pos / absP.z;
  }
  cubePos = clamp(cubePos * 0.5f + 0.5f, 0.f, 1.f);
  uv.xy = clamp(uv.xy * 0.5f + 0.5f, 0.f, 1.f);
  return uv.xy;
}

uvec2 Pcg2D(uvec2 v)
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

vec2 Hash2D(uvec2 u)
{
  u = Pcg2D(u);
  return (u >> 8u) * 5.9604644775390625e-08f * 2.f - 1.f;
}

uvec3 Pcg3D(uvec3 v)
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
  u = Pcg3D(u);
  return (u >> 8u) * 5.9604644775390625e-08f * 2.f - 1.f;
}

vec2 HashProjectToFace(vec3 hash, uint planeID)
{
  switch (planeID)
  {
  case 0:
  case 2:
    return hash.yz;
  case 1:
  case 3:
    return hash.xz;
  case 4:
  case 5:
    return hash.xy;
  }
}

uvec3 Extract(const uint extent3, const int offset)
{
  return uvec3(bitfieldExtract(extent3, offset + 0, 1),
               bitfieldExtract(extent3, offset + 1, 1),
               bitfieldExtract(extent3, offset + 2, 1));
}

uvec3 Extract1(const uint extent3) { return Extract(extent3, 0); }
uvec3 Extract2(const uint extent3) { return Extract(extent3, 3); }
uvec3 Extract3(const uint extent3) { return Extract(extent3, 6); }

// Accepts an unsigned vec2
vec3 PerlinNoise(const uvec2 uv, const vec2 uvFract, const uvec3 cubePos, const uvec3 extent, const uint extent3, const uint planeID)
{
  vec2 ga = Hash2D(uv + uvec2(0, 0));
  vec2 gb = Hash2D(uv + uvec2(1, 0));
  vec2 gc = Hash2D(uv + uvec2(0, 1));
  vec2 gd = Hash2D(uv + uvec2(1, 1));

  if (uv.x == extent.x) // Min x
  {
    vec3 ga3 = Hash3D(cubePos);
    vec3 gc3 = Hash3D(cubePos + Extract2(extent3));

    ga = HashProjectToFace(ga3, planeID);
    gc = HashProjectToFace(gc3, planeID);
  }
  else if (uv.x == extent.y - 1) // Max x
  {
    vec3 gb3 = Hash3D(cubePos + Extract1(extent3));
    vec3 gd3 = Hash3D(cubePos + Extract3(extent3));

    gb = HashProjectToFace(gb3, planeID);
    gd = HashProjectToFace(gd3, planeID);
  }
  if (uv.y == 0) // Min y
  {
    vec3 ga3 = Hash3D(cubePos);
    vec3 gb3 = Hash3D(cubePos + Extract1(extent3));

    ga = HashProjectToFace(ga3, planeID);
    gb = HashProjectToFace(gb3, planeID);
  }
  else if (uv.y == extent.z - 1) // Max y
  {
    vec3 gc3 = Hash3D(cubePos + Extract2(extent3));
    vec3 gd3 = Hash3D(cubePos + Extract3(extent3));

    gc = HashProjectToFace(gc3, planeID);
    gd = HashProjectToFace(gd3, planeID);
  }

  float va = dot(ga, uvFract - vec2(0.f, 0.f));
  float vb = dot(gb, uvFract - vec2(1.f, 0.f));
  float vc = dot(gc, uvFract - vec2(0.f, 1.f));
  float vd = dot(gd, uvFract - vec2(1.f, 1.f));

  vec2 u = uvFract * uvFract * uvFract * (uvFract * (uvFract * 6.f - 15.f) + 10.f);
  vec2 du = 30.f * uvFract * uvFract * (uvFract * (uvFract - 2.f) + 1.f);

  return vec3(va + u.x*(vb-va) + u.y*(vc-va) + u.x*u.y*(va-vb-vc+vd),  // Value
              ga + u.x*(gb-ga) + u.y*(gc-ga) + u.x*u.y*(ga-gb-gc+gd) + // Derivative
              du * (u.yx*(va-vb-vc+vd) + vec2(vb,vc) - va));
}

vec3 Terrain(vec2 p, vec3 cubePosF, const uint planeID, const uint extent3, const vec3 normal)
{
  p.x += planeID; // ]planeID, planeID + 1[
  p *= float(fbmFrequency); // ]planeID * fbmFrequency, (planeID + 1) * fbmFrequency[
  uvec3 extent = uvec3(planeID * fbmFrequency, (planeID + 1) * fbmFrequency, fbmFrequency); // .x = x_min, .y = x_max, .z = y_max
  vec2 uvFract = fract(p);
  uvec2 uv = uvec2(p);

  vec3 an = vec3(0.f);
  float b = 1.f;
  vec2 d = vec2(0.f);

  cubePosF *= fbmFrequency;
  vec3 cubeFract = fract(cubePosF);
  uvec3 cubePos = uvec3(cubePosF);
  for (uint i = 0; i < 10; i++)
  {
    vec3 n = PerlinNoise(uv, uvFract, cubePos, extent, extent3, planeID);

    n.yz *= amplitudePercent;

    float erosion = 1.f + dot(d, d);
    //float erosion = 1.f;
    an.yz += n.yz / erosion;

    d += n.yz;

    float erosion2 = 1.f + dot(d, d);
    //float erosion2 = 1.f;
    an.x += b * n.x / erosion2;

    b *= 0.5f;
    extent <<= 1;

    uv <<= 1;
    uvFract *= 2.f;
    uv += uvec2(uvFract);
    uvFract = fract(uvFract);

    cubePos <<= 1;
    cubeFract *= 2.f;
    cubePos += uvec3(cubeFract);
    cubeFract = fract(cubeFract);
  }
  an.yz *= float(fbmFrequency);
  return an;
}

vec3 SubgroupNormal(vec3 v)
{
  uint baseVertex = gl_SubgroupInvocationID & (~3u);
  vec3 v0 = subgroupShuffle(v, baseVertex);
  vec3 v1 = subgroupShuffle(v, baseVertex + 1);
  vec3 v2 = subgroupShuffle(v, baseVertex + 2);
  return normalize(cross(v1 - v0, v2 - v0));
}

vec3 ProjectOrthogonal(vec3 dir, vec3 proj)
{
  return dir - dot(dir, proj) * proj;
}

vec3 AlignToNormal(const vec2 dir, const vec3 normal, const uint planeID)
{
  vec3 tangent;
  vec3 bitangent;
  if (planeID == 0 || planeID == 2)
  {
    tangent = ProjectOrthogonal(vec3(0.f, 1.f, 0.f), normal);
    bitangent = ProjectOrthogonal(vec3(0.f, 0.f, 1.f), normal);
  }
  else if (planeID == 1 || planeID == 3)
  {
    tangent = ProjectOrthogonal(vec3(1.f, 0.f, 0.f), normal);
    bitangent = ProjectOrthogonal(vec3(0.f, 0.f, 1.f), normal);
  }
  else
  {
    tangent = ProjectOrthogonal(vec3(1.f, 0.f, 0.f), normal);
    bitangent = ProjectOrthogonal(vec3(0.f, 1.f, 0.f), normal);
  }

  vec3 gradient = dir.x * tangent + dir.y * bitangent;
  return normalize(normal - gradient);
}

#endif // DISPLACEMENT_GLSL
