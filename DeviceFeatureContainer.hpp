#pragma once

#include "Libs.hpp"

#include "InitializerList.hpp"

static inline bool VkBoolIsSubset(const VkBool32* b1, const VkBool32* b2,
                                  uint64_t count)
{
  for (uint64_t i = 0; i < count; i++)
  {
    if (b1[i] > b2[i])
    {
      return false;
    }
  }
  return true;
}

struct DeviceFeature
{
  std::vector<uint8_t> body;

  template <class T>
  constexpr DeviceFeature(T feature) : body(sizeof(T))
  {
    memcpy(body.data(), &feature, sizeof(feature));
  }

  constexpr void SetPNext(void* pNext) noexcept
  {
    const uint32_t offset = offsetof(VkPhysicalDeviceFeatures2, pNext);
    assert(body.size() > offset + sizeof(pNext));
    memcpy(&body[offset], &pNext, sizeof(pNext));
  }

  constexpr void* GetPNext() const noexcept
  {
    const uint32_t offset = offsetof(VkPhysicalDeviceFeatures2, pNext);
    assert(body.size() >= offset + sizeof(void*));
    void* pNext = (void*)*(uint64_t*)&body[offset];
    return pNext;
  }

  bool IsSubset(const DeviceFeature& other) const noexcept
  {
    const uint32_t offset = offsetof(VkPhysicalDeviceFeatures2, features);
    assert(body.size() == other.body.size() && body.size() > offset);

    return VkBoolIsSubset((VkBool32*)&body[offset],
                          (VkBool32*)&other.body[offset],
                          (body.size() - offset) / sizeof(VkBool32));
  }
};

class DeviceFeatureContainer
{
public:
  constexpr DeviceFeatureContainer() noexcept
  {
    mFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  }

  DeviceFeatureContainer(const DeviceFeatureContainer& other)
  {
    mFeatures = other.mFeatures;
    memcpy(&mFeatures2, &other.mFeatures2, sizeof(mFeatures2));

    GenPNextChain();
  }

  void Reserve(uint32_t size) noexcept { mFeatures.reserve(size); }

  void operator=(DeviceFeatureContainer&) = delete;

  template <class T>
  constexpr DeviceFeatureContainer& AddFeature(const T& feature) noexcept
  {
    mFeatures.push_back(feature);
    GenPNextChain();
    return *this;
  }

  bool IsSubset(const DeviceFeatureContainer& other) const noexcept
  {
    assert(mFeatures.size() == other.mFeatures.size());

    if (!VkBoolIsSubset((VkBool32*)&mFeatures2.features,
                        (VkBool32*)&other.mFeatures2.features,
                        sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32)))
    {
      return false;
    }

    for (uint32_t i = 0; i < mFeatures.size(); i++)
    {
      if (!mFeatures[i].IsSubset(other.mFeatures[i]))
      {
        return false;
      }
    }

    return true;
  }

  constexpr VkPhysicalDeviceFeatures2& GetFeatures2() noexcept
  {
    return mFeatures2;
  }

  constexpr void GenPNextChain() noexcept
  {
    if (mFeatures.size() == 0)
    {
      mFeatures2.pNext = nullptr;
      return;
    }

    mFeatures2.pNext = mFeatures[0].body.data();

    for (uint32_t i = 0; i < mFeatures.size() - 1; i++)
    {
      mFeatures[i].SetPNext(mFeatures[i + 1].body.data());
    }

    mFeatures.back().SetPNext(nullptr);
  }

private:
  std::vector<DeviceFeature> mFeatures;
  VkPhysicalDeviceFeatures2 mFeatures2{};
};
