#include "Buffers.hpp"

#include "Context.hpp"
#include "GpuMemAlloc.hpp"
#include "ResultCheck.hpp"
#include "vulkan/vulkan_core.h"

bool CreateBuffer(Buffer& buffer, const void* memAllocPNext) noexcept
{
  assert(buffer.size != 0);
  assert(buffer.usageFlags != 0);
  assert(buffer.memProperties != 0);

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = buffer.size;
  bufferInfo.usage = buffer.usageFlags;
  IfNRetFM(vkCreateBuffer(gContext.GetDevice(), &bufferInfo, nullptr,
                          &buffer.handle),
           "Failed to create buffer!");

  VkMemoryRequirements memRequirements{};
  vkGetBufferMemoryRequirements(gContext.GetDevice(), buffer.handle,
                                &memRequirements);

  if (!AllocateDeviceMemory(memRequirements, buffer.memProperties, buffer.mem,
                            memAllocPNext))
  {
    return false;
  }

  IfNRetFM(
      vkBindBufferMemory(gContext.GetDevice(), buffer.handle, buffer.mem, 0),
      "Failed to bind buffer memory!");

  return true;
}

void DestroyBuffer(Buffer& buffer) noexcept
{
  if (buffer.handle != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(gContext.GetDevice(), buffer.handle, nullptr);
    buffer.handle = VK_NULL_HANDLE;
  }

  if (buffer.mem != VK_NULL_HANDLE)
  {
    vkFreeMemory(gContext.GetDevice(), buffer.mem, nullptr);
    buffer.mem = VK_NULL_HANDLE;
  }
}

bool BufferCopyToHost(const Buffer& buffer, const void* data, VkDeviceSize size,
                      VkDeviceSize offset) noexcept
{
  assert(buffer.memProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
         buffer.memProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* mappedMem{nullptr};
  IfNRetFM(vkMapMemory(gContext.GetDevice(), buffer.mem, offset, size, 0,
                       &mappedMem),
           "Failed to map memory!");
  IfNRetF(
      memcpy(mappedMem, data, (size == VK_WHOLE_SIZE) ? buffer.size : size));
  vkUnmapMemory(gContext.GetDevice(), buffer.mem);
  return true;
}

bool BufferCopyFromHost(const Buffer& buffer, void* data, VkDeviceSize size,
                        VkDeviceSize offset) noexcept
{
  assert(buffer.memProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
         buffer.memProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* mappedMem{nullptr};
  IfNRetFM(vkMapMemory(gContext.GetDevice(), buffer.mem, offset, size, 0,
                       &mappedMem),
           "Failed to map memory!");
  IfNRetF(
      memcpy(data, mappedMem, (size == VK_WHOLE_SIZE) ? buffer.size : size));
  vkUnmapMemory(gContext.GetDevice(), buffer.mem);
  return true;
}

bool BufferClearHost(const Buffer& buffer, VkDeviceSize size,
                     VkDeviceSize offset) noexcept
{
  assert(buffer.memProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
         buffer.memProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void* mappedMem{nullptr};
  IfNRetFM(vkMapMemory(gContext.GetDevice(), buffer.mem, offset, size, 0,
                       &mappedMem),
           "Failed to map memory!");
  IfNRetF(memset(mappedMem, 0, (size == VK_WHOLE_SIZE) ? buffer.size : size));
  vkUnmapMemory(gContext.GetDevice(), buffer.mem);
  return true;
}

bool CreateStagingBufferAndLoad(Buffer& outStagingBuffer, const void* data,
                                VkDeviceSize size) noexcept
{
  outStagingBuffer.size = size;
  outStagingBuffer.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  outStagingBuffer.memProperties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  IfNRetF(CreateBuffer(outStagingBuffer));

  IfNRetF(BufferCopyToHost(outStagingBuffer, data, size));
  return true;
}

void BufferCopyDeviceToDevice(const Buffer& srcBuffer, const Buffer& dstBuffer,
                              VkCommandBuffer cmdBuffer, VkDeviceSize size,
                              VkDeviceSize srcOffset,
                              VkDeviceSize dstOffset) noexcept
{
  VkBufferCopy bufferCopy{
      .srcOffset = srcOffset, .dstOffset = dstOffset, .size = size};
  vkCmdCopyBuffer(cmdBuffer, srcBuffer.handle, dstBuffer.handle, 1,
                  &bufferCopy);
}

bool BufferCopyToDevice(const Buffer& buffer, VkCommandBuffer cmdBuffer,
                        Buffer& outStagingBuffer, const void* data,
                        VkDeviceSize size, VkDeviceSize offset) noexcept
{
  IfNRetF(CreateStagingBufferAndLoad(
      outStagingBuffer, data, (size == VK_WHOLE_SIZE) ? buffer.size : size));

  BufferCopyDeviceToDevice(outStagingBuffer, buffer, cmdBuffer,
                           (size == VK_WHOLE_SIZE) ? buffer.size : size, 0,
                           offset);
  return true;
}
