#pragma once

#include "Libs.hpp"

glm::vec2 CubeProj(const glm::vec3& pos, uint32_t& planeID, uint32_t& offset3,
                   glm::vec3& cubePos) noexcept;

float FbmPerlin(glm::vec2 p, glm::vec3 cubePosF, const uint32_t planeID,
                const uint32_t extent3, const uint32_t fbmFrequency,
                const float amplitudePercent,
                const uint8_t iterations) noexcept;
