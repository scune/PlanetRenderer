#version 460
#extension GL_EXT_debug_printf : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_null_initializer : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in flat uint inVertexID;

layout(binding = 1) uniform sampler2D Textures[];

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

void RotateMsc(inout vec3 worldPos, float offset)
{
  float blendSharpness = 10.f;

  vec3 blending = abs(inNormal.xyz);
  blending = pow(blending, vec3(blendSharpness));

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

vec3 TriplanarProjection(uint textureID, vec3 worldPos)
{
  float blendSharpness = 10.f;

  vec3 blending = abs(inNormal.xyz);
  blending = pow(blending, vec3(blendSharpness));

  float weightSum = blending.x + blending.y + blending.z;
  blending /= weightSum;

  vec3 color[3];
  color[0] = texture(Textures[textureID], worldPos.yz).rgb;
  color[1] = texture(Textures[textureID], worldPos.xz).rgb;
  color[2] = texture(Textures[textureID], worldPos.xy).rgb;

  return color[0] * blending.x + color[1] * blending.y + color[2] * blending.z;
}

vec3 TerrainColor()
{
  float slope = dot(normalize(inPos.xyz), inNormal.xyz);
  slope -= 0.75f;
  slope *= 4.f;
  slope = max(0.f, slope);
  //return vec3(slope);
  bool rock = (slope < 0.45f);
  bool textBlend = (!rock && slope < 0.5f);
  float textBlendAlpha = (rock) ? 1.f : 0.f;
  if (textBlend)
  {
    textBlendAlpha = 1.f - min(1.f, (slope - 0.45f) * 25.f);
    textBlendAlpha = min(1.f, exp(textBlendAlpha) - 1.f);
  }

  float scaling = 100.f;
  float scaling2 = 2.f;
  vec3 worldPos = inPos.xyz / scaling;

  float alpha = PerlinNoise3D(worldPos * 0.2f + 1100.f);
  vec3 offset = Hash3D(uvec3(worldPos / 50.f + 200.f));

  vec3 offsetWorldPos = worldPos;
  RotateMsc(offsetWorldPos, offset.x);
  offsetWorldPos *= 0.6f * (offset.y + 0.5f);
  offsetWorldPos += 0.7f * (offset.z + 0.5f);

  float rockAlpha = (rock) ? 1.f : 0.f;
  if (textBlend)
  {
    float rockHeight = TriplanarProjection(1, worldPos / scaling2).x * (1.f - alpha);
    float grassHeight = TriplanarProjection(3, worldPos).x * (1.f - alpha);

    rockHeight += TriplanarProjection(1, offsetWorldPos / scaling2).x * alpha;
    grassHeight += TriplanarProjection(3, offsetWorldPos).x * alpha;

    rockAlpha = (rockHeight > grassHeight) ? textBlendAlpha : 0.f;
  }

  vec3 color = vec3(0.f);
  if (rockAlpha > 0.f)
  {
    color += TriplanarProjection(0, worldPos / scaling2) * (1.f - alpha) * rockAlpha;
    color += TriplanarProjection(0, offsetWorldPos / scaling2) * alpha * rockAlpha;
  }
  if (rockAlpha < 1.f)
  {
    color += TriplanarProjection(2, worldPos) * (1.f - alpha) * (1.f - rockAlpha);
    color += TriplanarProjection(2, offsetWorldPos) * alpha * (1.f - rockAlpha);
  }
  return color;
}

void main()
{
  // Swapchain = vec4(UintToColor(PcgHash(inVertexID)) + vec3(0.f, 0.f, 0.5f), 1.f);
  Swapchain = vec4(TriplanarProjection(2, inPos / 100.f), 1.f);
  //Swapchain = vec4(TerrainColor(), 1.f);
  // Swapchain = vec4(CubeProj(normalize(inPos.xyz)) * inNormal.xy + 0.3f, 0.f, 1.f);
  // Swapchain = vec4(CubeProj(normalize(inPos.xyz)), 0.f, 1.f);
  //Swapchain = vec4(inNormal.xyz, 1.f);
}
