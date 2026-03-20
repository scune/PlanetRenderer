#include "GpuMemAlloc.hpp"

#include "Context.hpp"
#include "ResultCheck.hpp"

inline bool FindMemoryType(uint32_t typeFilter,
                           VkMemoryPropertyFlags memoryProperties,
                           uint32_t& memType) noexcept
{
  VkPhysicalDeviceMemoryProperties memProperties{};
  vkGetPhysicalDeviceMemoryProperties(gContext.GetPhysicalDevice(),
                                      &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
  {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    memoryProperties) == memoryProperties)
    {
      memType = i;
      return true;
    }
  }

  std::cerr << "Failed to find suitable memory type!\n";
  return false;
}

bool AllocateDeviceMemory(VkMemoryRequirements& memRequirements,
                          VkMemoryPropertyFlags memProperties,
                          VkDeviceMemory& mem,
                          const void* memAllocPNext) noexcept
{
  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.pNext = memAllocPNext;
  if (!FindMemoryType(memRequirements.memoryTypeBits, memProperties,
                      allocInfo.memoryTypeIndex))
  {
    return false;
  }

  IfNRetFM(vkAllocateMemory(gContext.GetDevice(), &allocInfo, nullptr, &mem),
           "Failed to allocate device memory!");

  return true;
}
