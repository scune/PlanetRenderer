#pragma once

#include <Libs.hpp>

#include "AllowedDescriptorInfos.hpp"
#include "Buffers.hpp"
#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "DescriptorSetLayout.hpp"
#include "Images.hpp"
#include "InitializerList.hpp"

template <uint32_t bindingCount>
class DescriptorBuilder
{
public:
  DescriptorBuilder() = default;
  ~DescriptorBuilder() = default;

  DescriptorBuilder(DescriptorBuilder&&) = delete;
  DescriptorBuilder(const DescriptorBuilder&) = delete;

  inline constexpr DescriptorBuilder&
  AddSSBO(const Buffer& buffer, VkShaderStageFlags stageFlags,
          bool bPerSwapchainImage = false) noexcept
  {
    const auto bufferInfo = BufferGetDescriptorInfo(buffer);
    return AddBinding<VkDescriptorBufferInfo>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                              stageFlags, {bufferInfo},
                                              bPerSwapchainImage);
  }

  inline constexpr DescriptorBuilder&
  AddUBO(const Buffer& buffer, VkShaderStageFlags stageFlags,
         bool bPerSwapchainImage = false) noexcept
  {
    const auto bufferInfo = BufferGetDescriptorInfo(buffer);
    return AddBinding<VkDescriptorBufferInfo>(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                              stageFlags, {bufferInfo},
                                              bPerSwapchainImage);
  }

  inline constexpr DescriptorBuilder&
  AddCombImgSampler(const Image& image, VkShaderStageFlags stageFlags,
                    bool bPerSwapchainImage = false) noexcept
  {
    const auto imageInfo = ImageGetDescriptorInfo(image);
    return AddBinding<VkDescriptorImageInfo>(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, {imageInfo},
        bPerSwapchainImage);
  }

  inline constexpr DescriptorBuilder&
  AddCombImgSampler(const std::vector<Image>& images,
                    VkShaderStageFlags stageFlags,
                    bool bPerSwapchainImage = false) noexcept
  {
    std::vector<VkDescriptorImageInfo> imageInfos{images.size()};
    for (uint32_t i = 0; i < images.size(); i++)
    {
      imageInfos[i] = ImageGetDescriptorInfo(images[i]);
    }

    return AddBinding<VkDescriptorImageInfo>(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, imageInfos,
        bPerSwapchainImage);
  }

  inline constexpr DescriptorBuilder&
  AddStorageImage(const Image& image, VkShaderStageFlags stageFlags,
                  bool bPerSwapchainImage = false) noexcept
  {
    const auto imageInfo = ImageGetDescriptorInfo(image);
    return AddBinding<VkDescriptorImageInfo>(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                             stageFlags, {imageInfo},
                                             bPerSwapchainImage);
  }

  inline constexpr DescriptorBuilder&
  AddAccelerationStructure(VkAccelerationStructureKHR tlasHandle,
                           VkShaderStageFlags stageFlags,
                           bool bPerSwapchainImage = false) noexcept
  {
    VkWriteDescriptorSetAccelerationStructureKHR asInfo{
        .sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .pNext = nullptr,
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &tlasHandle};
    return AddBinding<VkWriteDescriptorSetAccelerationStructureKHR>(
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, stageFlags, {asInfo},
        bPerSwapchainImage);
  }

  inline constexpr DescriptorBuilder& AddNullDescriptor() noexcept
  {
    mBindingIdx++;
    return *this;
  }

  template <AllowedDescriptors::Infos T>
  inline constexpr DescriptorBuilder&
  AddBinding(VkDescriptorType type, VkShaderStageFlags stageFlags,
             InitializerList<T> resourceInfos, bool bPerSwapchainImage) noexcept
  {
    assert(resourceInfos.size() > 0);
    assert(mBindingIdx < bindingCount);

    mBindings[mBindingIdx].binding = mBindingIdx;
    mBindings[mBindingIdx].descriptorType = type;
    mBindings[mBindingIdx].descriptorCount =
        (bPerSwapchainImage) ? 1 : resourceInfos.size();
    mBindings[mBindingIdx].stageFlags = stageFlags;

    if (resourceInfos.size() == 1)
    {
      mResourceInfos[mBindingIdx] = DescriptorInfoTypes(resourceInfos[0]);
    }
    else
    {
      mResourceInfos[mBindingIdx] =
          std::vector<T>(resourceInfos.begin(), resourceInfos.end());
    }

    mBindingIdx++;

    return *this;
  }

  inline std::expected<VkDescriptorSetLayout, VkResult>
  BuildDescriptorSetLayout() noexcept
  {
    VkDescriptorBindingFlags flags[bindingCount];
    for (uint32_t i = 0; i < bindingCount; i++)
    {
      flags[i] = VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
    }

    return CreateDescriptorSetLayout(InitializerList{mBindings, bindingCount},
                                     InitializerList{flags, bindingCount});
  }

  inline std::expected<VkDescriptorPool, VkResult>
  BuildDescriptorPool(uint32_t maxSets = 1) noexcept
  {
    std::array<VkDescriptorPoolSize, bindingCount> poolSizes;
    for (uint32_t i = 0; i < bindingCount; i++)
    {
      poolSizes[i].type = mBindings[i].descriptorType;
      poolSizes[i].descriptorCount = mBindings[i].descriptorCount * maxSets;
    }

    return CreateDescriptorPool(
        InitializerList{poolSizes.data(), poolSizes.size()}, maxSets);
  }

  inline bool BuildDescriptorSet(const VkDescriptorPool& pool,
                                 const VkDescriptorSetLayout& setLayout,
                                 DescriptorSet& descriptorSet)
  {
    return descriptorSet.AllocateDescriptorSets(
        pool, setLayout, InitializerList{mBindings, bindingCount});
  }

  inline const DescriptorResourceInfo* GetResourceInfos() const noexcept
  {
    return mResourceInfos;
  }

private:
  uint32_t mBindingIdx{};
  VkDescriptorSetLayoutBinding mBindings[bindingCount]{};
  DescriptorResourceInfo mResourceInfos[bindingCount]{};
};
