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

float MinComp(vec2 v)
{
  return min(v.x, v.y);
}

float GridBoundingDist(vec3 gridPos)
{
  vec3 absN = abs(inNormal.xyz);
  if (absN.x > absN.y && absN.x > absN.z)
  {
    gridPos.xy = gridPos.yz;
  }
  else if (absN.y > absN.z)
  {
    gridPos.xy = gridPos.xz;
  }
  gridPos.xy = fract(abs(gridPos.xy));
  float minDist = MinComp(gridPos.xy);
  minDist = min(minDist, MinComp(vec2(1.f) - gridPos.xy));
  return minDist;
}

vec3 TerrainColorWithOffset(vec3 offsetWorldPos, float scaling2, float textBlendAlpha)
{
  float rockAlpha = textBlendAlpha;

  vec3 color = vec3(0.f);
  if (rockAlpha > 0.f)
  {
    color += TriplanarProjection(0, offsetWorldPos / scaling2) * rockAlpha;
  }
  if (rockAlpha < 1.f)
  {
    color += TriplanarProjection(2, offsetWorldPos) * (1.f - rockAlpha);
  }
  return color;
}

vec3 TerrainColor()
{
  const float slopeRockCutoff = 0.5f;
  const float slopeRockBlendCutoff = 0.55f;

  float slope = dot(normalize(inPos.xyz), inNormal.xyz);
  bool rock = (slope < slopeRockCutoff);
  bool textBlend = (!rock && slope < slopeRockBlendCutoff);
  float textBlendAlpha = (rock) ? 1.f : 0.f;
  if (textBlend)
  {
    textBlendAlpha = 1.f - min(1.f, (slope - slopeRockCutoff)) / (slopeRockBlendCutoff - slopeRockCutoff);
  }

  vec3 gridPos = inPos.xyz / 10.f; //
  vec3 offset = Hash3D(uvec3(gridPos + 10000.f)); //
  float gridBoundDist = GridBoundingDist(gridPos);

  float scaling = 2.f; //
  float scaling2 = 2.f;
  vec3 worldPos = inPos.xyz / scaling;

  vec3 offsetWorldPos = worldPos;
  RotateMsc(offsetWorldPos, offset.x);
  offsetWorldPos *= 0.6f * (offset.y + 0.5f);
  offsetWorldPos += 0.7f * (offset.z + 0.5f);

  vec3 color = TerrainColorWithOffset(offsetWorldPos, scaling2, textBlendAlpha);

  float defaultCutoff = 0.2f;
  if (gridBoundDist > defaultCutoff)
    return color;

  vec3 offsetWorldPos2 = worldPos * 0.5f + 0.5f; // Offset by 0.5 so the cell edges are in the other cells

  float defaultColorAlpha = 1.f - (gridBoundDist / defaultCutoff);
  vec3 defaultColor = TerrainColorWithOffset(offsetWorldPos2, scaling2, textBlendAlpha);
  return mix(color, defaultColor, defaultColorAlpha);
}

void main()
{
  //Swapchain = vec4(UintToColor(PcgHash(inVertexID)) + vec3(0.f, 0.f, 0.5f), 1.f);
  Swapchain = vec4(TerrainColor(), 1.f);
  //Swapchain = vec4(CubeProj(normalize(inPos.xyz)), 0.f, 1.f);
  //Swapchain = vec4(inNormal.xyz * 0.5f + 0.5f, 1.f);
  //Swapchain = vec4(inNormal.xyz, 1.f);
  //Swapchain = vec4(dot(inNormal.xyz, normalize(inPos.xyz)));
}
