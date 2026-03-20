#pragma once

#include "Libs.hpp"
#include "vulkan/vulkan_core.h"

struct Buffer
{
  VkBuffer handle{VK_NULL_HANDLE};
  VkDeviceMemory mem{VK_NULL_HANDLE};

  VkDeviceSize size{0};
  VkBufferUsageFlags usageFlags{(VkBufferUsageFlags)0};
  VkMemoryPropertyFlags memProperties{(VkMemoryPropertyFlags)0};
};

bool CreateBuffer(Buffer& buffer, const void* memAllocPNext = nullptr) noexcept;

void DestroyBuffer(Buffer& buffer) noexcept;

bool BufferCopyToHost(const Buffer& buffer, const void* data,
                      VkDeviceSize size = VK_WHOLE_SIZE,
                      VkDeviceSize offset = 0) noexcept;

bool BufferCopyFromHost(const Buffer& buffer, void* data,
                        VkDeviceSize size = VK_WHOLE_SIZE,
                        VkDeviceSize offset = 0) noexcept;

bool BufferClearHost(const Buffer& buffer, VkDeviceSize size = VK_WHOLE_SIZE,
                     VkDeviceSize offset = 0) noexcept;

bool CreateStagingBufferAndLoad(Buffer& outStagingBuffer, const void* data,
                                VkDeviceSize size) noexcept;

void BufferCopyDeviceToDevice(const Buffer& srcBuffer, const Buffer& dstBuffer,
                              VkCommandBuffer cmdBuffer, VkDeviceSize size,
                              VkDeviceSize srcOffset = 0,
                              VkDeviceSize dstOffset = 0) noexcept;

bool BufferCopyToDevice(const Buffer& buffer, VkCommandBuffer cmdBuffer,
                        Buffer& outStagingBuffer, const void* data,
                        VkDeviceSize size = VK_WHOLE_SIZE,
                        VkDeviceSize offset = 0) noexcept;

inline VkDescriptorBufferInfo
BufferGetDescriptorInfo(const Buffer& buffer, VkDeviceSize offset = 0) noexcept
{
  VkDescriptorBufferInfo info{};
  info.buffer = buffer.handle;
  info.offset = offset;
  info.range = buffer.size;
  return info;
}
