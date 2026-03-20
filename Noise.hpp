#pragma once

#include "Libs.hpp"

#include "Images.hpp"
#include <expected>

// Converts a random uint to a random float in [0, 1).
// https://github.com/rust-random/rand/blob/7aa25d577e2df84a5156f824077bb7f6bdf28d97/src/distributions/float.rs#L111-L117
static inline float UintToUnitFloat(const uint i) noexcept
{
  // 32 - 8 = 24 (exponent), 1.0 / (1 << 24) = 5.9604644775390625e-08
  // top bits are usually more random.
  return (i >> 8) * 5.9604644775390625e-08;
}

static inline uint XORShift(uint32_t seed) noexcept
{
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}

static inline float XORShiftUnitFloat(uint32_t seed) noexcept
{
  seed = XORShift(seed);
  return UintToUnitFloat(seed);
}

static inline uint32_t PcgHash(uint32_t seed) noexcept
{
  uint32_t state = seed * 747796405u + 2891336453u;
  uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  seed = (word >> 22u) ^ word;
  return seed;
}

static inline uint32_t PcgHash() noexcept
{
  static uint32_t seed{73856093};
  seed = PcgHash(seed);
  return seed;
}

std::expected<Image, bool> Gen2DNoiseTexture(const VkExtent2D extent) noexcept;
