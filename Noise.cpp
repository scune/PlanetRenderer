#include "Noise.hpp"

#include "Textures.hpp"

std::expected<Image, bool> Gen2DNoiseTexture(const VkExtent2D extent) noexcept
{
  std::vector<uint8_t> noiseMap(extent.width * extent.height);

  uint32_t count = noiseMap.size() / (4);
  for (uint32_t i = 0; i < count; i++)
  {
    noiseMap[PcgHash() % noiseMap.size()] = 255;
  }

  auto txtRes = Textures::LoadFromMem(noiseMap.data(), extent, 1);
  if (txtRes.has_value())
  {
    return txtRes.value();
  }
  else
  {
    return std::unexpected(false);
  }
}
