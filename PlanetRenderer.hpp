#pragma once

#include "Libs.hpp"

#include "Buffers.hpp"
#include "CameraBase.hpp"
#include "CbtGpuBisection.hpp"
#include "DescriptorSet.hpp"
#include "Images.hpp"

class PlanetRenderer
{
public:
  PlanetRenderer() = default;
  ~PlanetRenderer() = default;

  PlanetRenderer(PlanetRenderer&&) = delete;
  PlanetRenderer(const PlanetRenderer&) = delete;

  void Init();
  void Destroy();

  void Update();

  void DrawCompute(VkCommandBuffer cmdBuffer, uint32_t swapchainImgIdx);
  void DrawGraphics(VkCommandBuffer cmdBuffer, uint32_t swapchainImgIdx);

private:
  std::vector<glm::vec4> mVertices;
  std::vector<uint32_t> mIndices;

  void CreateBuffers();
  Buffer mVertexBuffer{};
  Buffer mIndexBuffer{};
  Buffer mSceneData{};
  Image mTextures[4]{};
  Image mDepthImage{};

  void CreateDescriptor();
  VkDescriptorPool mDescriptorPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout mDescriptorSetLayout{VK_NULL_HANDLE};
  DescriptorSet mDescriptorSet;

  void CreateComputeShader();
  void CreateGraphicsShader();
  std::array<VkShaderEXT, 2> mShader{VK_NULL_HANDLE};
  VkPipelineLayout mLayout{VK_NULL_HANDLE};

  CameraBase* mCam{nullptr};
  bool mbPlayerCam{true};
  bool mbCamSwitchButtonPressed;

  CbtBisection mCbtBisection{};

  struct SceneData_t
  {
    glm::mat4 camMatrix;
  };

  PFN_vkCmdBindShadersEXT vkCmdBindShadersEXT{nullptr};
  PFN_vkCmdSetPolygonModeEXT vkCmdSetPolygonModeEXT{nullptr};
  PFN_vkCmdSetRasterizationSamplesEXT vkCmdSetRasterizationSamplesEXT{nullptr};
  PFN_vkCmdSetSampleMaskEXT vkCmdSetSampleMaskEXT{nullptr};
  PFN_vkCmdSetAlphaToCoverageEnableEXT vkCmdSetAlphaToCoverageEnableEXT{
      nullptr};
  PFN_vkCmdSetColorBlendEnableEXT vkCmdSetColorBlendEnableEXT{nullptr};
  PFN_vkCmdSetColorWriteMaskEXT vkCmdSetColorWriteMaskEXT{nullptr};
  PFN_vkCmdSetVertexInputEXT vkCmdSetVertexInputEXT{nullptr};
};
