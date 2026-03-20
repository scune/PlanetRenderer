vec2 UvUnwrap(const vec3 n)
{
  const vec3 absN = abs(n);
  uint dominantSide = (absN.x > absN.y) ? 0 : 1;
  dominantSide = (absN[dominantSide] > absN.z) ? dominantSide : 2;

  vec3 uv = n / n[dominantSide]; // Project to plane
  uv = uv * 0.5f + 0.5f;

  // return vec2(uv[(dominantSide + 1) & 2], uv[(dominantSide + 2) & 2]);

  if (n[dominantSide] > 0)
    uv.z = 1.f - uv.z;

  if (dominantSide == 0)
  {
    uv.xy = uv.yz;
  }
  else if (dominantSide == 1)
  {
    uv.x = 1.f - uv.x;
    uv.xy = uv.xz;
  }
  else
  {
    if (n[dominantSide] < 0)
      uv.y = 1.f - uv.y;
    uv.xy =  uv.yx;
  }

  uv.x += dominantSide;
  uv.y += (n[dominantSide] < 0) ? 1.f : 0.f;
  uv.xy /= vec2(3.f, 2.f);
  return uv.xy;
}
