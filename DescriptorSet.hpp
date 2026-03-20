#pragma once

#include "Libs.hpp"

#include <cstring>
#include <variant>

#include "AllowedDescriptorInfos.hpp"
#include "Context.hpp"
#include "InitializerList.hpp"

using DescriptorInfoTypes =
    std::variant<VkDescriptorImageInfo, VkDescriptorBufferInfo, VkBufferView,
                 VkWriteDescriptorSetAccelerationStructureKHR>;
using DescriptorResourceInfo =
    std::variant<DescriptorInfoTypes, std::vector<VkDescriptorImageInfo>,
                 std::vector<VkDescriptorBufferInfo>, std::vector<VkBufferView>,
                 std::vector<VkWriteDescriptorSetAccelerationStructureKHR>>;

class DescriptorSet
{
public:
  DescriptorSet() = default;

  inline bool AllocateDescriptorSets(
      const VkDescriptorPool& pool, const VkDescriptorSetLayout& setLayout,
      InitializerList<VkDescriptorSetLayoutBinding> bindings) noexcept
  {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.pSetLayouts = &setLayout;
    allocInfo.descriptorSetCount = 1;
    if (vkAllocateDescriptorSets(gContext.GetDevice(), &allocInfo, &mSet) !=
        VK_SUCCESS)
    {
      return false;
    }

    mWrites.resize(bindings.size());
    for (uint32_t i = 0; i < bindings.size(); i++)
    {
      mWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      mWrites[i].dstSet = mSet;
      mWrites[i].dstBinding = i;
      mWrites[i].descriptorCount = bindings[i].descriptorCount;
      mWrites[i].descriptorType = bindings[i].descriptorType;
    }
    return true;
  }

  inline bool Update(const DescriptorResourceInfo* resourceInfos,
                     uint32_t swapchainIdx = 0) noexcept
  {
    assert(mWrites.size() > 0);

    bool ret{true};

    for (uint32_t i = 0; i < mWrites.size(); i++)
    {
      if (mWrites[i].descriptorCount == 0)
        continue;

      if (const auto infoTypes =
              std::get_if<DescriptorInfoTypes>(&resourceInfos[i]))
      {
        if (const auto imageInfo =
                std::get_if<VkDescriptorImageInfo>(infoTypes))
        {
          ret = ret && SetWriteResource(imageInfo, 1, i);
        }
        else if (const auto bufferInfo =
                     std::get_if<VkDescriptorBufferInfo>(infoTypes))
        {
          ret = ret && SetWriteResource(bufferInfo, 1, i);
        }
        else if (const auto bufferView = std::get_if<VkBufferView>(infoTypes))
        {
          ret = ret && SetWriteResource(bufferView, 1, i);
        }
        else if (const auto asInfo =
                     std::get_if<VkWriteDescriptorSetAccelerationStructureKHR>(
                         infoTypes))
        {
          ret = ret && SetWriteResource(asInfo, 1, i);
        }
      }
      else if (const auto imageInfos =
                   std::get_if<std::vector<VkDescriptorImageInfo>>(
                       &resourceInfos[i]))
      {
        ret = ret && SetWriteResource((mWrites[i].descriptorCount == 1)
                                          ? &(*imageInfos)[swapchainIdx]
                                          : imageInfos->data(),
                                      mWrites[i].descriptorCount, i);
      }
      else if (const auto bufferInfos =
                   std::get_if<std::vector<VkDescriptorBufferInfo>>(
                       &resourceInfos[i]))
      {
        ret = ret && SetWriteResource((mWrites[i].descriptorCount == 1)
                                          ? &(*bufferInfos)[swapchainIdx]
                                          : bufferInfos->data(),
                                      mWrites[i].descriptorCount, i);
      }
      else if (const auto bufferViews =
                   std::get_if<std::vector<VkBufferView>>(&resourceInfos[i]))
      {
        ret = ret && SetWriteResource((mWrites[i].descriptorCount == 1)
                                          ? &(*bufferViews)[swapchainIdx]
                                          : bufferViews->data(),
                                      mWrites[i].descriptorCount, i);
      }
      else if (const auto asInfos = std::get_if<
                   std::vector<VkWriteDescriptorSetAccelerationStructureKHR>>(
                   &resourceInfos[i]))
      {
        ret = ret && SetWriteResource((mWrites[i].descriptorCount == 1)
                                          ? &(*asInfos)[swapchainIdx]
                                          : asInfos->data(),
                                      mWrites[i].descriptorCount, i);
      }
    }

    if (!ret)
      return false;

    vkUpdateDescriptorSets(gContext.GetDevice(), mWrites.size(), mWrites.data(),
                           0, nullptr);

    return true;
  }

  inline const VkDescriptorSet& Get() const noexcept { return mSet; }
  inline const std::vector<VkWriteDescriptorSet>& GetWrites() const noexcept
  {
    return mWrites;
  }

private:
  template <AllowedDescriptors::Infos T>
  inline bool SetWriteResource(const T* data, uint32_t count,
                               uint32_t writeIndex) noexcept
  {
    bool ret{true};
    for (uint32_t i = 0; i < count; i++)
    {
      T empty{};
      ret = ret && (memcmp(&data[i], &empty, sizeof(T)) != 0);
    }

    if constexpr (std::same_as<T, VkDescriptorImageInfo>)
    {
      mWrites[writeIndex].pImageInfo = data;
    }
    else if constexpr (std::same_as<T, VkDescriptorBufferInfo>)
    {
      mWrites[writeIndex].pBufferInfo = data;
    }
    else if constexpr (std::same_as<T, VkBufferView>)
    {
      mWrites[writeIndex].pTexelBufferView = data;
    }
    else if constexpr (std::same_as<
                           T, VkWriteDescriptorSetAccelerationStructureKHR>)
    {
      mWrites[writeIndex].pNext = data;
    }

    return ret;
  }

  VkDescriptorSet mSet{VK_NULL_HANDLE};
  std::vector<VkWriteDescriptorSet> mWrites;
};
