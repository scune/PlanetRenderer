#pragma once

#include "Libs.hpp"

#include "Images.hpp"
#include "ResultCheck.hpp"
#include <expected>

namespace Textures
{
enum Error
{
  FILE_COULD_NOT_BE_OPENED,
  NO_SUPPORTED_COMPONENTS,
  FAILED_TO_CREATE_IMAGE,
  FAILED_TO_COPY_TO_IMAGE,
  FAILED_TO_CREATE_SAMPLER,
  FAILED_TO_TRANSITION_IMAGE
};

std::expected<Image, Error>
LoadFromMem(const void* data, VkExtent2D extent, uint8_t components,
            VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VkImageUsageFlags additionalUsageFlags =
                VK_IMAGE_USAGE_SAMPLED_BIT) noexcept;

std::expected<Image, Error>
LoadFromFile(const char* path,
             VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
             VkImageUsageFlags additionalUsageFlags =
                 VK_IMAGE_USAGE_SAMPLED_BIT) noexcept;

std::string ErrToStr(Error err) noexcept;

inline void TextIfNThrow(const std::expected<Image, Error>& res)
{
  if (!res.has_value())
  {
    std::string errStr = "Failed to load texture: ";
    errStr += ErrToStr(res.error());
    errStr += "!";
    IfNThrow(false, errStr.c_str());
  }
}
} // namespace Textures
