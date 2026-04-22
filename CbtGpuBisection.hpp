#pragma once

#include "Libs.hpp"

#include "Buffers.hpp"
#include "DescriptorSet.hpp"
#include "Images.hpp"
#include "Shaders/Cbt/Bisector.glsl.hpp"
#include "Shaders/Cbt/Halfedge.glsl.hpp"

class CbtBisection
{
public:
  CbtBisection() = default;

  void Init(uint8_t maxSubdivision, InitializerList<Cbt::Halfedge> halfedges,
            const Buffer& vertexBuffer, float scale,
            InitializerList<Image> textures);
  void Destroy();

  void Update(const glm::vec3& camPos, const glm::mat4& camMatrix,
              const glm::vec3& lookAt);

  void RecordCmds(VkCommandBuffer cmdBuffer);
  void VertShaderPipeBarrier(VkCommandBuffer cmdBuffer) noexcept;

  inline const Buffer& GetVertexCacheBuffer() const noexcept
  {
    return mVertexCache;
  }

private:
  bool mNoUpdate{false};
  bool mLastFramePressed{false};

  void RootBisectorVertices(uint32_t halfedgeID,
                            InitializerList<Cbt::Halfedge> halfedges,
                            InitializerList<glm::vec4> rootVertices,
                            glm::vec3* outVecs) const noexcept;

  void InitConstPushConstantData(uint8_t maxSubdivision,
                                 InitializerList<Cbt::Halfedge> halfedges,
                                 float scale) noexcept;

  void InitBuffers(const uint32_t halfedgeCount);

  void CreateBuffers(const Cbt::Halfedge* halfedges,
                     const Cbt::Bisector* bisectors, const uint32_t count);
  Buffer mCbt{};
  Buffer mPointerCache{};
  Buffer mDispatchBuffer{};
  Buffer mBisectors{};
  Buffer mHalfedges{};
  Buffer mAllocCounter{};
  Buffer mCommands{};
  Buffer mVertexCache{};
  Buffer mGlobalUpdate{};
  Buffer mDebugBuffer{};

  void CreateDescriptors(const Buffer& vertexBuffer,
                         InitializerList<Image> textures);
  VkDescriptorPool mIndirectDispatchPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout mIndirectDispatchSetLayout{VK_NULL_HANDLE};
  DescriptorSet mIndirectDispatchSet{};

  VkDescriptorPool mDescriptorPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout mDescriptorSetLayout{VK_NULL_HANDLE};
  DescriptorSet mDescriptorSet{};

  void
  CreatePipelineLayouts(const VkPushConstantRange& sumReducPushConstantRange,
                        const VkPushConstantRange& genCmdsPushConstantRange);
  void
  CreateComputeShaders(const VkPushConstantRange& sumReducPushConstantRange,
                       const VkPushConstantRange& genCmdsPushConstantRange);
  VkShaderEXT mIndirectDispatchShader{VK_NULL_HANDLE};
  VkPipelineLayout mIndirectDispatchLayout{VK_NULL_HANDLE};

  VkShaderEXT mPointerCacheShader{VK_NULL_HANDLE};
  VkShaderEXT mResetCommandsShader{VK_NULL_HANDLE};

  VkShaderEXT mClassifyShader{VK_NULL_HANDLE};
  VkShaderEXT mGenerateCmdsShader{VK_NULL_HANDLE};
  VkShaderEXT mValidateCmdsShader{VK_NULL_HANDLE};
  VkPipelineLayout mGenerateCmdsLayout{VK_NULL_HANDLE};

  VkShaderEXT mReserveBlocksShader{VK_NULL_HANDLE};
  VkShaderEXT mFillNewBlocksShader{VK_NULL_HANDLE};
  VkShaderEXT mUpdateNeighboursShader{VK_NULL_HANDLE};
  VkShaderEXT mUpdateBitfieldShader{VK_NULL_HANDLE};

  VkShaderEXT mSumReductionShader{VK_NULL_HANDLE};
  VkPipelineLayout mSumReductionLayout{VK_NULL_HANDLE};

  VkShaderEXT mCacheVertexShader{VK_NULL_HANDLE};

  VkShaderEXT mIndirectDispatchVertexShader{VK_NULL_HANDLE};
  VkShaderEXT mPlanetDisplacementShader{VK_NULL_HANDLE};

  VkPipelineLayout mLayout{VK_NULL_HANDLE};

  struct SumReducPushConstants
  {
    uint32_t depth;
  };
  VkPushConstantRange mSumReducPushConstantRange{};

  struct GenCmdsPushConstants
  {
    uint32_t maxSubdivision;
    uint32_t minDepth;
    uint32_t baseBisectorIndex;
    uint32_t pixelCount;
    float planetScale;
    uint32_t fbmFrequency;
    float fbmAmplitude;
    float amplitudePercent;
  } mGenCmdsPushConstants{};
  VkPushConstantRange mGenCmdsPushConstantRange{};

  struct GlobalUpdate_t
  {
    glm::mat4 camMatrix;
    glm::vec4 frustumPlanes[4];
    glm::vec4 camPos;
    glm::vec4 lookAt;
  };

  void FillIndirectDispatchBuffer(VkCommandBuffer cmdBuffer);
  void CachePointers(VkCommandBuffer cmdBuffer);
  void ResetCommands(VkCommandBuffer cmdBuffer);
  void Classify(VkCommandBuffer cmdBuffer);
  void GenerateCmds(VkCommandBuffer cmdBuffer);
  void ValidateCmds(VkCommandBuffer cmdBuffer);
  void ReserveBlocks(VkCommandBuffer cmdBuffer);
  void FillNewBlocks(VkCommandBuffer cmdBuffer);
  void UpdateNeighbours(VkCommandBuffer cmdBuffer);
  void UpdateBitfield(VkCommandBuffer cmdBuffer);
  void SumReduction(VkCommandBuffer cmdBuffer);
  void CacheVertices(VkCommandBuffer cmdBuffer);
  void PlanetDisplacement(VkCommandBuffer cmdBuffer);

  PFN_vkCmdBindShadersEXT vkCmdBindShadersEXT{nullptr};
};
