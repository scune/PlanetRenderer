#version 460
#extension GL_EXT_debug_printf : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_null_initializer : require
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in flat uint inVertexID;

layout(binding = 1) uniform sampler2D texture1;
layout(binding = 2) uniform sampler2D texture2;

layout(location = 0) out vec4 Swapchain;



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
  return (i >> 8u) * 5.9604644775390625e-08;
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
  return (Pcg3d(u) >> 8u) * 5.9604644775390625e-08;
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

// Accepts unsigned x
float PerlinNoise3D(vec3 x)
{
  uvec3 p = uvec3(x);
  vec3 w = fract(x);

  vec3 u = w*w*w*(w*(w*6.0-15.0)+10.0);

  float a = Hash3D1D(p + uvec3(0, 0, 0));
  float b = Hash3D1D(p + uvec3(1, 0, 0));
  float c = Hash3D1D(p + uvec3(0, 1, 0));
  float d = Hash3D1D(p + uvec3(1, 1, 0));
  float e = Hash3D1D(p + uvec3(0, 0, 1));
  float f = Hash3D1D(p + uvec3(1, 0, 1));
  float g = Hash3D1D(p + uvec3(0, 1, 1));
  float h = Hash3D1D(p + uvec3(1, 1, 1));

  float k0 =   a;
  float k1 =   b - a;
  float k2 =   c - a;
  float k3 =   e - a;
  float k4 =   a - b - c + d;
  float k5 =   a - c - e + g;
  float k6 =   a - b - e + f;
  float k7 = - a + b + c - d + e - f - g + h;

  return k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z;
}

void RotateMsc(inout vec3 worldPos, const vec3 blending, float offset)
{
  float rotation = 60.f * (offset + 0.5f);
  if (blending.x > blending.y && blending.x > blending.z)
  {
    worldPos.yz = Rotate(worldPos.yz, rotation);
  }
  else if (blending.y > blending.z)
  {
    worldPos.xz = Rotate(worldPos.xz, rotation);
  }
  else
  {
    worldPos.xy = Rotate(worldPos.xy, rotation);
  }
}

vec3 TriplanarProjection(float blendSharpness)
{
  //float slope = inNormal.z;
  //bool rock = (slope < 0.6f);
  bool rock = false;

  vec3 blending = abs(inNormal.xyz); // TODO
  //vec3 blending = abs(inNormal.xyz);
  blending = pow(blending, vec3(blendSharpness));

  float weightSum = blending.x + blending.y + blending.z;
  blending /= weightSum;

  vec3 worldPos = inPos.xyz / ((rock) ? 50.f : 10.f);

  float alpha = PerlinNoise3D(worldPos * 0.2f + 1100.f);
  vec3 offset = Hash3D(uvec3(worldPos / 50.f + 200.f));
  //return vec3(offset);
  //return vec3(alpha);

  vec3 color[3];
  if (rock)
  {
    color[0] = texture(texture1, worldPos.yz).rgb * (1.f - alpha);
    color[1] = texture(texture1, worldPos.xz).rgb * (1.f - alpha);
    color[2] = texture(texture1, worldPos.xy).rgb * (1.f - alpha);
  }
  else
  {
    color[0] = texture(texture2, worldPos.yz).rgb * (1.f - alpha);
    color[1] = texture(texture2, worldPos.xz).rgb * (1.f - alpha);
    color[2] = texture(texture2, worldPos.xy).rgb * (1.f - alpha);
  }

  RotateMsc(worldPos, blending, offset.x);
  worldPos *= 0.6f * (offset.y + 0.5f);
  worldPos += 0.7f * (offset.z + 0.5f);

  if (rock)
  {
    color[0] += texture(texture1, worldPos.yz).rgb * alpha;
    color[1] += texture(texture1, worldPos.xz).rgb * alpha;
    color[2] += texture(texture1, worldPos.xy).rgb * alpha;
  }
  else
  {
    color[0] += texture(texture2, worldPos.yz).rgb * alpha;
    color[1] += texture(texture2, worldPos.xz).rgb * alpha;
    color[2] += texture(texture2, worldPos.xy).rgb * alpha;
  }

  return color[0] * blending.x + color[1] * blending.y + color[2] * blending.z;
}

void main()
{
  // Swapchain = vec4(UintToColor(PcgHash(inVertexID)) + vec3(0.f, 0.f, 0.5f), 1.f);
  //Swapchain = vec4(TriplanarProjection(10.f), 1.f);
  // Swapchain = vec4(CubeProj(normalize(inPos.xyz)) * inNormal.xy + 0.3f, 0.f, 1.f);
  // Swapchain = vec4(CubeProj(normalize(inPos.xyz)), 0.f, 1.f);
  Swapchain = vec4(inNormal.xyz, 1.f);
}
