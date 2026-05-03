#pragma once

#include "Libs.hpp"

#include "Device.hpp"
#include "InitializerList.hpp"

class Context
{
public:
  Context() = default;
  ~Context() = default;

  Context(const Context&) = delete;
  void operator()(const Context&) = delete;

  void Init(bool force_x11);
  void CreateCmdBufferPreRec(uint32_t count);
  void Destroy() noexcept;

  bool WindowShouldClose() noexcept;

  void UpdateWindowTitle() noexcept;

  bool FrameCap() noexcept;

  [[nodiscard]] VkResult WaitForFence(VkFence fence) noexcept;
  [[nodiscard]] VkResult ResetFence(VkFence fence) noexcept;

  [[nodiscard]] bool ResetAndBeginCmdBufferOneTime() noexcept;
  [[nodiscard]] VkResult EndCmdBufferOneTime() noexcept;

  void BeginCmdBufferPreRec(uint32_t idx);
  void EndCmdBufferPreRec(uint32_t idx);

  void SubmitWorkPreRec(uint32_t idx);
  [[nodiscard]] bool OneTimeSubmit() noexcept;

  VkFormat FindSupportedFormat(InitializerList<VkFormat> candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features) const noexcept;

  inline uint32_t GetDispatchCount1D(uint32_t count) const noexcept
  {
    return (count + mWorkgroupSize1D - 1) / mWorkgroupSize1D;
  }

  inline glm::uvec2 GetDispatchCount2D(VkExtent2D extent) const
  {
    return glm::uvec2((extent.width + mWorkgroupSize2D - 1) / mWorkgroupSize2D,
                      (extent.height + mWorkgroupSize2D - 1) /
                          mWorkgroupSize2D);
  }

  inline GLFWwindow* GetWindow() const noexcept { return mWindow; }

  inline double GetDeltaTime() const noexcept { return mDeltaTime; }
  inline uint32_t GetFrameCounter() const noexcept { return mFrameCounter; }

  inline VkInstance GetInstance() const noexcept { return mInstance; }
  inline VkSurfaceKHR GetSurface() const noexcept { return mSurface; }

  inline VkDevice GetDevice() const noexcept { return mDevice; }
  inline VkPhysicalDevice GetPhysicalDevice() const noexcept
  {
    return mPhysicalDevice;
  }

  inline VkQueue GetGraphicsQueue() const noexcept { return mGraphicsQueue; }
  inline VkQueue GetPresentQueue() const noexcept { return mPresentQueue; }

  inline const QueueFamilyIndices& GetQueueFamilyIndices() const noexcept
  {
    return mQueueFamilyIndices;
  }

  inline VkCommandBuffer GetCmdBufferOneTime() const noexcept
  {
    return mCmdBufferOneTime;
  }

  inline VkFence GetFencePreRec() const noexcept { return mFencePreRec; }

  inline VkCommandBuffer GetCmdBufferPreRec(uint32_t idx) const noexcept
  {
    return mCmdBufferPreRec[idx];
  }

  inline bool HostImageTransferSupp() const noexcept
  {
    return mHostImageTransfer;
  }

  inline const VkPhysicalDeviceProperties& GetDeviceProperties() const noexcept
  {
    return mDeviceProps;
  }

private:
  void CreateWindow(bool force_x11);
  bool m_bGlfwInit{false};
  static constexpr char mWindowTitle[]{"VEGA Engine"};
  static constexpr char mWindowArgTitle[]{"VEGA Engine {:>4} Fps"};
  GLFWwindow* mWindow{nullptr};

  void NewDeltaTime() noexcept;
  double UpdateDeltaTime() noexcept;
  double mLastFrameTime{};
  double mDeltaTime{};
  double mTitleUpdateTimer{};
  // double mMaxFrameTime{1. / 144.};
  double mMaxFrameTime{0.};
  uint32_t mFrameCounter{};

#ifdef DEBUG
  bool CheckValidationLayerSupport() noexcept;
  bool SetupDebugMessenger() noexcept;
  VkDebugUtilsMessengerEXT mDebugMessenger{VK_NULL_HANDLE};
#endif // DEBUG

  void CreateVulkan();
  VkInstance mInstance{VK_NULL_HANDLE};
  VkSurfaceKHR mSurface{VK_NULL_HANDLE};

  VkPhysicalDevice mPhysicalDevice{VK_NULL_HANDLE};
  VkDevice mDevice{VK_NULL_HANDLE};

  QueueFamilyIndices mQueueFamilyIndices{};
  VkQueue mGraphicsQueue{VK_NULL_HANDLE};
  VkQueue mPresentQueue{VK_NULL_HANDLE};

  void CreateCommandBuffer();
  VkCommandPool mCmdPoolOneTime{VK_NULL_HANDLE};
  VkCommandBuffer mCmdBufferOneTime{VK_NULL_HANDLE};

  VkCommandPool mCmdPoolPreRec{VK_NULL_HANDLE};
  std::vector<VkCommandBuffer> mCmdBufferPreRec{VK_NULL_HANDLE};

  VkFence mFenceOneTime{VK_NULL_HANDLE};
  VkFence mFencePreRec{VK_NULL_HANDLE};

  void GetDeviceProps() noexcept;
  uint32_t mWorkgroupSize1D{1};
  uint32_t mWorkgroupSize2D{1};
  bool mHostImageTransfer{false};
  VkPhysicalDeviceProperties mDeviceProps;
};

inline Context gContext;
