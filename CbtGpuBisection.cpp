#include "CbtGpuBisection.hpp"

#include "Context.hpp"
#include "DescriptorBuilder.hpp"
#include "GetVkFunc.hpp"
#include "Noise.hpp"
#include "ResultCheck.hpp"
#include "Shader.hpp"
#include "Shaders/Cbt/Generic.glsl.hpp"
#include "Shaders/PlanetVertex.glsl.hpp"
#include "Swapchain.hpp"

inline uint32_t FastFloorLog2(uint32_t i) noexcept
{
  return sizeof(i) * 8 - 1 - __builtin_clz(i); // MSB
}

inline uint32_t MinDepth(uint32_t halfedgeCount) noexcept
{
  return FastFloorLog2(halfedgeCount) + 1;
}

// clang-format off
const glm::mat3 m0 = glm::transpose(glm::mat3{
    0.f,   0.f, 1.f,
    1.f,   0.f, 0.f,
    0.5f, 0.5f, 0.f});
const glm::mat3 m1 = glm::transpose(glm::mat3{
    0.f,   1.f, 0.f,
    0.f,   0.f, 1.f,
    0.5f, 0.5f, 0.f});
// clang-format on

void M0M1_Test()
{
  glm::mat3 m32 = glm::identity<glm::mat3>();
  std::vector<glm::mat3> fp32mats((1 << 5));
  for (uint32_t i = 0; i < (1 << 5); i++)
  {
    m32 *= (i & 1) ? m1 : m0;
    fp32mats[i] = m32;
  }

  glm::dmat3 m64 = glm::identity<glm::dmat3>();
  std::vector<glm::dmat3> fp64mats((1 << 5));
  for (uint32_t i = 0; i < (1 << 5); i++)
  {
    m64 *= (i & 1) ? glm::dmat3(m1) : glm::dmat3(m0);
    fp64mats[i] = m64;
  }

  m32 *= fp32mats[0b10101];
  m32 *= fp32mats[0b10111];
  m32 *= fp32mats[0b00011];
  m32 *= fp32mats[0b01100];

  m64 *= fp64mats[0b10101];
  m64 *= fp64mats[0b10111];
  m64 *= fp64mats[0b00011];
  m64 *= fp64mats[0b01100];

  COUT_MAT3(m32);
  COUT_MAT3(m64);
}

void CbtBisection::RootBisectorVertices(
    uint32_t halfedgeID, InitializerList<Cbt::Halfedge> halfedges,
    InitializerList<glm::vec4> rootVertices, glm::vec3* outVecs) const noexcept
{
  // halfedgeID -= mGenCmdsPushConstants.baseBisectorIndex;
  uint32_t next = halfedges[halfedgeID].next;

  outVecs[0] = glm::vec3(rootVertices[halfedges[halfedgeID].vert]);
  outVecs[1] = glm::vec3(rootVertices[halfedges[next].vert]);
  outVecs[2] = outVecs[0];

  float n = 1.f;
  uint32_t h = next;
  while (h != halfedgeID)
  {
    outVecs[2] += glm::vec3(rootVertices[halfedges[h].vert]);
    n++;
    h = halfedges[h].next;
  }
  outVecs[2] /= n;
}

void CbtBisection::InitConstPushConstantData(
    uint32_t maxSubdivision, InitializerList<Cbt::Halfedge> halfedges,
    float scale) noexcept
{
  mGenCmdsPushConstants.maxSubdivision =
      std::min<uint32_t>(CBT_DEPTH, maxSubdivision);
  mGenCmdsPushConstants.minDepth = MinDepth(halfedges.size());
  mGenCmdsPushConstants.baseBisectorIndex = 1 << mGenCmdsPushConstants.minDepth;
  mGenCmdsPushConstants.pixelCount =
      gSwapchain.GetExtent().width * gSwapchain.GetExtent().height;
  mGenCmdsPushConstants.planetScale = scale;
  mGenCmdsPushConstants.baseFbmFrequency = 4.f;
  mGenCmdsPushConstants.baseFbmAmplitude = 2.f;

  assert(mGenCmdsPushConstants.minDepth + mGenCmdsPushConstants.maxSubdivision <
         64);
}

inline std::vector<Cbt::Bisector>
CreateRootBisectors(InitializerList<Cbt::Halfedge> halfedges,
                    const uint32_t baseBisectorIndex)
{
  std::vector<Cbt::Bisector> bisectors(halfedges.size());
  for (uint32_t i = 0; i < halfedges.size(); i++)
  {
    bisectors[i] = Cbt::InitializeBisector();
    bisectors[i].next = halfedges[i].next;
    bisectors[i].prev = halfedges[i].prev;
    bisectors[i].twin = halfedges[i].twin;
    bisectors[i].index = baseBisectorIndex + i;
  }
  return bisectors;
}

void CbtBisection::Init(uint32_t maxSubdivision,
                        InitializerList<Cbt::Halfedge> halfedges,
                        const Buffer& vertexBuffer, float scale)
{
  InitConstPushConstantData(maxSubdivision, halfedges, scale);

  const auto bisectors =
      CreateRootBisectors(halfedges, mGenCmdsPushConstants.baseBisectorIndex);

  CreateBuffers(halfedges.begin(), bisectors.data(), halfedges.size());
  CreateDescriptors(vertexBuffer);

  mSumReducPushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  mSumReducPushConstantRange.offset = 0;
  mSumReducPushConstantRange.size = sizeof(SumReducPushConstants);

  mGenCmdsPushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  mGenCmdsPushConstantRange.offset = sizeof(SumReducPushConstants);
  mGenCmdsPushConstantRange.size = sizeof(GenCmdsPushConstants);

  CreatePipelineLayouts(mSumReducPushConstantRange, mGenCmdsPushConstantRange);
  CreateComputeShaders(mSumReducPushConstantRange, mGenCmdsPushConstantRange);

  InitBuffers(halfedges.size());
}

void CbtBisection::CreateBuffers(const Cbt::Halfedge* halfedges,
                                 const Cbt::Bisector* bisectors,
                                 const uint32_t count)
{
  const VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  mCbt.size =
      sizeof(uint32_t) * CBT_TREE_SIZE + sizeof(uint64_t) * CBT_BITFIELD_SIZE;
  mCbt.usageFlags =
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  mCbt.memProperties = memProps;
  IfNThrow(CreateBuffer(mCbt), "Failed to create cbt buffer!");

  mPointerCache.size = sizeof(uint32_t) * BISECTOR_MAX_COUNT;
  mPointerCache.usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  mPointerCache.memProperties = memProps;
  IfNThrow(CreateBuffer(mPointerCache),
           "Failed to create pointer cache buffer!");

  mDispatchBuffer.size = sizeof(VkDispatchIndirectCommand);
  mDispatchBuffer.usageFlags =
      VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  mDispatchBuffer.memProperties = memProps;
  IfNThrow(CreateBuffer(mDispatchBuffer), "Failed to create dispatch buffer!");

  mBisectors.size = sizeof(Cbt::Bisector) * BISECTOR_MAX_COUNT;
  mBisectors.usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  mBisectors.memProperties = memProps;
  IfNThrow(CreateBuffer(mBisectors), "Failed to create bisector buffer!");

  IfNThrow(
      BufferCopyToHost(mBisectors, bisectors, sizeof(Cbt::Bisector) * count),
      "Failed to copy to bisector buffer!");

  mHalfedges.size = sizeof(Cbt::Halfedge) * count;
  mHalfedges.usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  mHalfedges.memProperties = memProps;
  IfNThrow(CreateBuffer(mHalfedges), "Failed to create halfedge buffer!");

  IfNThrow(
      BufferCopyToHost(mHalfedges, halfedges, sizeof(Cbt::Halfedge) * count),
      "Failed to copy to halfedge buffer!");

  mAllocCounter.size = sizeof(int32_t);
  mAllocCounter.usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  mAllocCounter.memProperties = memProps;
  IfNThrow(CreateBuffer(mAllocCounter),
           "Failed to create allocation counter buffer!");

  mCommands.size = sizeof(uint32_t) * BISECTOR_MAX_COUNT;
  mCommands.usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  mCommands.memProperties = memProps;
  IfNThrow(CreateBuffer(mCommands),
           "Failed to create bisector command buffer!");

  mVertexCache.size = sizeof(Cbt::PlanetVertex) * 3 * BISECTOR_MAX_COUNT +
                      sizeof(VkDrawIndirectCommand);
  mVertexCache.usageFlags =
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
  mVertexCache.memProperties = memProps;
  IfNThrow(CreateBuffer(mVertexCache), "Failed to create vertex cache buffer!");

  mGlobalUpdate.size = sizeof(GlobalUpdate_t);
  mGlobalUpdate.usageFlags =
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  mGlobalUpdate.memProperties = memProps;
  IfNThrow(CreateBuffer(mGlobalUpdate),
           "Failed to create global update buffer!");

  mDebugBuffer.size = sizeof(glm::ivec2) * 10000;
  mDebugBuffer.usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  mDebugBuffer.memProperties = memProps;
  IfNThrow(CreateBuffer(mDebugBuffer), "Failed to create debug buffer!");
}

void CbtBisection::CreateDescriptors(const Buffer& vertexBuffer)
{
  {
    DescriptorBuilder<2> builder;
    builder.AddSSBO(mCbt, VK_SHADER_STAGE_COMPUTE_BIT);
    builder.AddSSBO(mDispatchBuffer, VK_SHADER_STAGE_COMPUTE_BIT);

    auto poolRes = builder.BuildDescriptorPool();
    if (poolRes.has_value())
    {
      mIndirectDispatchPool = poolRes.value();
    }
    else
    {
      IfNThrow(poolRes.error(), "Failed to create descriptor pool!");
    }

    auto setLayoutRes = builder.BuildDescriptorSetLayout();
    if (setLayoutRes.has_value())
    {
      mIndirectDispatchSetLayout = setLayoutRes.value();
    }
    else
    {
      IfNThrow(setLayoutRes.error(), "Failed to create descriptor pool!");
    }

    IfNThrow(builder.BuildDescriptorSet(mIndirectDispatchPool,
                                        mIndirectDispatchSetLayout,
                                        mIndirectDispatchSet),
             "Failed to create descriptor set!");
    IfNThrow(mIndirectDispatchSet.Update(builder.GetResourceInfos()),
             "Failed to update descriptor set!");
  }

  DescriptorBuilder<11> builder;
  builder.AddSSBO(mCbt, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddSSBO(mPointerCache, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddSSBO(mBisectors, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddSSBO(mHalfedges, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddSSBO(mAllocCounter, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddSSBO(vertexBuffer, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddSSBO(mCommands, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddSSBO(mVertexCache, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddUBO(mGlobalUpdate, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddSSBO(mDebugBuffer, VK_SHADER_STAGE_COMPUTE_BIT);
  builder.AddSSBO(mDispatchBuffer, VK_SHADER_STAGE_COMPUTE_BIT);

  auto poolRes = builder.BuildDescriptorPool();
  if (poolRes.has_value())
  {
    mDescriptorPool = poolRes.value();
  }
  else
  {
    IfNThrow(poolRes.error(), "Failed to create descriptor pool!");
  }

  auto setLayoutRes = builder.BuildDescriptorSetLayout();
  if (setLayoutRes.has_value())
  {
    mDescriptorSetLayout = setLayoutRes.value();
  }
  else
  {
    IfNThrow(setLayoutRes.error(), "Failed to create descriptor pool!");
  }

  IfNThrow(builder.BuildDescriptorSet(mDescriptorPool, mDescriptorSetLayout,
                                      mDescriptorSet),
           "Failed to create descriptor set!");
  IfNThrow(mDescriptorSet.Update(builder.GetResourceInfos()),
           "Failed to update descriptor set!");
}

void CbtBisection::CreatePipelineLayouts(
    const VkPushConstantRange& mSumReducPushConstantRange,
    const VkPushConstantRange& mGenCmdsPushConstantRange)
{
  std::array<VkPipelineLayout, 4> layouts{};
  std::array<VkPipelineLayoutCreateInfo, 4> createInfos{};
  for (auto& createInfo : createInfos)
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

  createInfos[0].setLayoutCount = 1;
  createInfos[0].pSetLayouts = &mIndirectDispatchSetLayout;

  createInfos[1].setLayoutCount = 1;
  createInfos[1].pSetLayouts = &mDescriptorSetLayout;

  createInfos[2].setLayoutCount = 1;
  createInfos[2].pSetLayouts = &mDescriptorSetLayout;
  createInfos[2].pushConstantRangeCount = 1;
  createInfos[2].pPushConstantRanges = &mGenCmdsPushConstantRange;

  createInfos[3].setLayoutCount = 1;
  createInfos[3].pSetLayouts = &mDescriptorSetLayout;
  createInfos[3].pushConstantRangeCount = 1;
  createInfos[3].pPushConstantRanges = &mSumReducPushConstantRange;

  static_assert(layouts.size() == createInfos.size());

  for (uint32_t i = 0; i < layouts.size(); i++)
  {
    IfNThrow(vkCreatePipelineLayout(gContext.GetDevice(), &createInfos[i],
                                    nullptr, &layouts[i]),
             "Failed to create pipeline layout!");
  }

  mIndirectDispatchLayout = layouts[0];
  mLayout = layouts[1];
  mGenerateCmdsLayout = layouts[2];
  mSumReductionLayout = layouts[3];
}

void CbtBisection::CreateComputeShaders(
    const VkPushConstantRange& mSumReducPushConstantRange,
    const VkPushConstantRange& mGenCmdsPushConstantRange)
{
  std::array<VkDescriptorSetLayout, 1> setLayouts{mDescriptorSetLayout};
  std::array<VkShaderEXT, 14> shaders{};
  std::array<VkShaderCreateInfoEXT, 14> shaderCreateInfos{};
  for (auto& createInfo : shaderCreateInfos)
  {
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
    createInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    createInfo.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
    createInfo.pName = "main";
  }

  GetVkFuncAuto(gContext.GetInstance(), vkCreateShadersEXT);

  std::vector<uint32_t> shaderCodeIndirectDispatch;
  Shader::LoadSPV("Cbt/IndirectDispatch.comp.spv", shaderCodeIndirectDispatch);
  shaderCreateInfos[0].codeSize = VecByteSize(shaderCodeIndirectDispatch);
  shaderCreateInfos[0].pCode = shaderCodeIndirectDispatch.data();
  shaderCreateInfos[0].setLayoutCount = 1;
  shaderCreateInfos[0].pSetLayouts = &mIndirectDispatchSetLayout;

  std::vector<uint32_t> shaderCodeCachePointers;
  Shader::LoadSPV("Cbt/CachePointers.comp.spv", shaderCodeCachePointers);
  shaderCreateInfos[1].codeSize = VecByteSize(shaderCodeCachePointers);
  shaderCreateInfos[1].pCode = shaderCodeCachePointers.data();
  shaderCreateInfos[1].setLayoutCount = setLayouts.size();
  shaderCreateInfos[1].pSetLayouts = setLayouts.data();

  std::vector<uint32_t> shaderCodeResetCmds;
  Shader::LoadSPV("Cbt/ResetCommands.comp.spv", shaderCodeResetCmds);
  shaderCreateInfos[2].codeSize = VecByteSize(shaderCodeResetCmds);
  shaderCreateInfos[2].pCode = shaderCodeResetCmds.data();
  shaderCreateInfos[2].setLayoutCount = setLayouts.size();
  shaderCreateInfos[2].pSetLayouts = setLayouts.data();

  std::vector<uint32_t> shaderCodeClassify;
  Shader::LoadSPV("Cbt/Classify.comp.spv", shaderCodeClassify);
  shaderCreateInfos[3].codeSize = VecByteSize(shaderCodeClassify);
  shaderCreateInfos[3].pCode = shaderCodeClassify.data();
  shaderCreateInfos[3].setLayoutCount = setLayouts.size();
  shaderCreateInfos[3].pSetLayouts = setLayouts.data();
  shaderCreateInfos[3].pushConstantRangeCount = 1;
  shaderCreateInfos[3].pPushConstantRanges = &mGenCmdsPushConstantRange;

  std::vector<uint32_t> shaderCodeGenCmds;
  Shader::LoadSPV("Cbt/GenerateCmds.comp.spv", shaderCodeGenCmds);
  shaderCreateInfos[4].codeSize = VecByteSize(shaderCodeGenCmds);
  shaderCreateInfos[4].pCode = shaderCodeGenCmds.data();
  shaderCreateInfos[4].setLayoutCount = setLayouts.size();
  shaderCreateInfos[4].pSetLayouts = setLayouts.data();
  shaderCreateInfos[4].pushConstantRangeCount = 1;
  shaderCreateInfos[4].pPushConstantRanges = &mGenCmdsPushConstantRange;

  std::vector<uint32_t> shaderCodeValidateCmds;
  Shader::LoadSPV("Cbt/ValidateCmds.comp.spv", shaderCodeValidateCmds);
  shaderCreateInfos[5].codeSize = VecByteSize(shaderCodeValidateCmds);
  shaderCreateInfos[5].pCode = shaderCodeValidateCmds.data();
  shaderCreateInfos[5].setLayoutCount = setLayouts.size();
  shaderCreateInfos[5].pSetLayouts = setLayouts.data();

  std::vector<uint32_t> shaderCodeReserveBlocks;
  Shader::LoadSPV("Cbt/ReserveBlocks.comp.spv", shaderCodeReserveBlocks);
  shaderCreateInfos[6].codeSize = VecByteSize(shaderCodeReserveBlocks);
  shaderCreateInfos[6].pCode = shaderCodeReserveBlocks.data();
  shaderCreateInfos[6].setLayoutCount = setLayouts.size();
  shaderCreateInfos[6].pSetLayouts = setLayouts.data();

  std::vector<uint32_t> shaderCodeFillNewBlocks;
  Shader::LoadSPV("Cbt/FillNewBlocks.comp.spv", shaderCodeFillNewBlocks);
  shaderCreateInfos[7].codeSize = VecByteSize(shaderCodeFillNewBlocks);
  shaderCreateInfos[7].pCode = shaderCodeFillNewBlocks.data();
  shaderCreateInfos[7].setLayoutCount = setLayouts.size();
  shaderCreateInfos[7].pSetLayouts = setLayouts.data();

  std::vector<uint32_t> shaderCodeUpdateNeighbours;
  Shader::LoadSPV("Cbt/UpdateNeighbours.comp.spv", shaderCodeUpdateNeighbours);
  shaderCreateInfos[8].codeSize = VecByteSize(shaderCodeUpdateNeighbours);
  shaderCreateInfos[8].pCode = shaderCodeUpdateNeighbours.data();
  shaderCreateInfos[8].setLayoutCount = setLayouts.size();
  shaderCreateInfos[8].pSetLayouts = setLayouts.data();

  std::vector<uint32_t> shaderCodeUpdateBitfield;
  Shader::LoadSPV("Cbt/UpdateBitfield.comp.spv", shaderCodeUpdateBitfield);
  shaderCreateInfos[9].codeSize = VecByteSize(shaderCodeUpdateBitfield);
  shaderCreateInfos[9].pCode = shaderCodeUpdateBitfield.data();
  shaderCreateInfos[9].setLayoutCount = setLayouts.size();
  shaderCreateInfos[9].pSetLayouts = setLayouts.data();

  std::vector<uint32_t> shaderCodeSumReduction;
  Shader::LoadSPV("Cbt/SumReduction.comp.spv", shaderCodeSumReduction);
  shaderCreateInfos[10].codeSize = VecByteSize(shaderCodeSumReduction);
  shaderCreateInfos[10].pCode = shaderCodeSumReduction.data();
  shaderCreateInfos[10].setLayoutCount = setLayouts.size();
  shaderCreateInfos[10].pSetLayouts = setLayouts.data();
  shaderCreateInfos[10].pushConstantRangeCount = 1;
  shaderCreateInfos[10].pPushConstantRanges = &mSumReducPushConstantRange;

  std::vector<uint32_t> shaderCodeCacheVertices;
  Shader::LoadSPV("Cbt/CacheVertices.comp.spv", shaderCodeCacheVertices);
  shaderCreateInfos[11].codeSize = VecByteSize(shaderCodeCacheVertices);
  shaderCreateInfos[11].pCode = shaderCodeCacheVertices.data();
  shaderCreateInfos[11].setLayoutCount = setLayouts.size();
  shaderCreateInfos[11].pSetLayouts = setLayouts.data();
  shaderCreateInfos[11].pushConstantRangeCount = 1;
  shaderCreateInfos[11].pPushConstantRanges = &mGenCmdsPushConstantRange;

  std::vector<uint32_t> shaderCodeIndDispVertex;
  Shader::LoadSPV("Cbt/IndirectDispatchVertex.comp.spv",
                  shaderCodeIndDispVertex);
  shaderCreateInfos[12].codeSize = VecByteSize(shaderCodeIndDispVertex);
  shaderCreateInfos[12].pCode = shaderCodeIndDispVertex.data();
  shaderCreateInfos[12].setLayoutCount = setLayouts.size();
  shaderCreateInfos[12].pSetLayouts = setLayouts.data();

  std::vector<uint32_t> shaderCodePlanetDisp;
  Shader::LoadSPV("Cbt/PlanetDisplacement.comp.spv", shaderCodePlanetDisp);
  shaderCreateInfos[13].codeSize = VecByteSize(shaderCodePlanetDisp);
  shaderCreateInfos[13].pCode = shaderCodePlanetDisp.data();
  shaderCreateInfos[13].setLayoutCount = setLayouts.size();
  shaderCreateInfos[13].pSetLayouts = setLayouts.data();
  shaderCreateInfos[13].pushConstantRangeCount = 1;
  shaderCreateInfos[13].pPushConstantRanges = &mGenCmdsPushConstantRange;

  static_assert(shaders.size() == shaderCreateInfos.size());
  IfNThrow(vkCreateShadersEXT(gContext.GetDevice(), shaders.size(),
                              shaderCreateInfos.data(), nullptr,
                              shaders.data()),
           "Failed to create shaders!");

  mIndirectDispatchShader = shaders[0];
  mPointerCacheShader = shaders[1];
  mResetCommandsShader = shaders[2];
  mClassifyShader = shaders[3];
  mGenerateCmdsShader = shaders[4];
  mValidateCmdsShader = shaders[5];
  mReserveBlocksShader = shaders[6];
  mFillNewBlocksShader = shaders[7];
  mUpdateNeighboursShader = shaders[8];
  mUpdateBitfieldShader = shaders[9];
  mSumReductionShader = shaders[10];
  mCacheVertexShader = shaders[11];
  mIndirectDispatchVertexShader = shaders[12];
  mPlanetDisplacementShader = shaders[13];

  vkCmdBindShadersEXT = GetVkFunc(gContext.GetInstance(), vkCmdBindShadersEXT);
}

void CbtBisection::InitBuffers(const uint32_t halfedgeCount)
{
  gContext.ResetAndBeginCmdBufferOneTime();
  auto cmdBuffer = gContext.GetCmdBufferOneTime();

  // Initialize cbt bitfield data
  const uint32_t bitFieldOffset = CBT_TREE_SIZE * sizeof(uint32_t);
  uint32_t fullSizeCount = sizeof(uint32_t) * (halfedgeCount / 64) * 2;
  if (fullSizeCount > 0)
  {
    vkCmdFillBuffer(cmdBuffer, mCbt.handle, bitFieldOffset, fullSizeCount, ~0u);
  }

  uint32_t rest = halfedgeCount % 64;
  if (rest != 0)
  {
    uint32_t offset = bitFieldOffset + fullSizeCount;
    uint64_t restBitfield = (1ull << rest) - 1;
    vkCmdFillBuffer(cmdBuffer, mCbt.handle, offset, sizeof(uint32_t),
                    restBitfield & 0xFFFFFFFF);
    offset += sizeof(uint32_t);
    vkCmdFillBuffer(cmdBuffer, mCbt.handle, offset, sizeof(uint32_t),
                    restBitfield >> 32);
  }

  VkBufferMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
  barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = mCbt.handle;
  barrier.offset = bitFieldOffset;
  barrier.size = sizeof(uint64_t) * ((halfedgeCount + 63) / 64);

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = 1;
  dependencyInfo.pBufferMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);

  SumReduction(cmdBuffer);

  FillIndirectDispatchBuffer(cmdBuffer);
  CachePointers(cmdBuffer);

  gContext.EndCmdBufferOneTime();
  gContext.OneTimeSubmit();
}

void ExtractFrustumPlanes(const glm::mat4& camMatrix, glm::vec4* planes)
{
  // Left Plane:   Row4 + Row1
  planes[0] = glm::vec4(
      camMatrix[0][3] + camMatrix[0][0], camMatrix[1][3] + camMatrix[1][0],
      camMatrix[2][3] + camMatrix[2][0], camMatrix[3][3] + camMatrix[3][0]);
  // Right Plane:  Row4 - Row1
  planes[1] = glm::vec4(
      camMatrix[0][3] - camMatrix[0][0], camMatrix[1][3] - camMatrix[1][0],
      camMatrix[2][3] - camMatrix[2][0], camMatrix[3][3] - camMatrix[3][0]);
  // Top Plane:    Row4 + Row2
  planes[2] = glm::vec4(
      camMatrix[0][3] + camMatrix[0][1], camMatrix[1][3] + camMatrix[1][1],
      camMatrix[2][3] + camMatrix[2][1], camMatrix[3][3] + camMatrix[3][1]);
  // Bottom Plane: Row4 - Row2
  planes[3] = glm::vec4(
      camMatrix[0][3] - camMatrix[0][1], camMatrix[1][3] - camMatrix[1][1],
      camMatrix[2][3] - camMatrix[2][1], camMatrix[3][3] - camMatrix[3][1]);

  for (uint32_t i = 0; i < 4; i++)
  {
    planes[i] /= glm::length(glm::vec3(planes[i]));
    // planes[i] = glm::normalize(planes[i]);
    // COUT(planes[i].w);
  }
}

void CbtBisection::Update(const glm::vec3& camPos, const glm::mat4& camMatrix,
                          const glm::vec3& lookAt)
{
  if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_H) == GLFW_PRESS &&
      !mLastFramePressed)
  {
    mNoUpdate = !mNoUpdate;
    mLastFramePressed = true;
  }
  else if (glfwGetKey(gContext.GetWindow(), GLFW_KEY_H) == GLFW_RELEASE)
  {
    mLastFramePressed = false;
  }

  if (!mNoUpdate)
  {
    GlobalUpdate_t data{};
    data.camMatrix = camMatrix;
    data.camPos = glm::vec4(camPos, 1.f);
    ExtractFrustumPlanes(camMatrix, data.frustumPlanes);
    data.lookAt = glm::vec4(lookAt, 1.f);
    IfNThrow(BufferCopyToHost(mGlobalUpdate, &data),
             "Failed to update global update buffer!");
  }

  // Debug:
  /*std::vector<glm::ivec2> debugData(10000);
  BufferCopyFromHost(mDebugBuffer, debugData.data(), VecByteSize(debugData));
  uint32_t maxIdx = 0;
  for (uint32_t i = 0; i < debugData.size(); i++)
  {
    if (debugData[i] != glm::ivec2(0))
    {
      COUT(i << ": " << debugData[i].x << ", " << debugData[i].y);
      maxIdx = std::max(maxIdx, i + 1);
    }
  }
  if (maxIdx != 0)
    BufferClearHost(mDebugBuffer, maxIdx * sizeof(glm::ivec2));*/
}

void CbtBisection::RecordCmds(VkCommandBuffer cmdBuffer)
{
  ResetCommands(cmdBuffer);
  Classify(cmdBuffer);
  GenerateCmds(cmdBuffer);
  ValidateCmds(cmdBuffer);
  ReserveBlocks(cmdBuffer);
  FillNewBlocks(cmdBuffer);
  UpdateNeighbours(cmdBuffer);
  UpdateBitfield(cmdBuffer);
  SumReduction(cmdBuffer);

  FillIndirectDispatchBuffer(cmdBuffer);
  CachePointers(cmdBuffer);
  CacheVertices(cmdBuffer);
  PlanetDisplacement(cmdBuffer);
}

void CbtBisection::FillIndirectDispatchBuffer(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mIndirectDispatchSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          mIndirectDispatchLayout, 0, 1, descriptorSets, 0,
                          nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mIndirectDispatchShader);

  vkCmdDispatch(cmdBuffer, 1, 1, 1);

  std::array<VkBufferMemoryBarrier2, 1> barriers{};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT |
                             VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].dstAccessMask =
      VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = mDispatchBuffer.handle;
  barriers[0].size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = barriers.size();
  dependencyInfo.pBufferMemoryBarriers = barriers.data();
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::CachePointers(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0,
                          1, descriptorSets, 0, nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mPointerCacheShader);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);

  VkBufferMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = mPointerCache.handle;
  barrier.size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = 1;
  dependencyInfo.pBufferMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::ResetCommands(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0,
                          1, descriptorSets, 0, nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mResetCommandsShader);

  // vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);
  vkCmdDispatch(cmdBuffer, (BISECTOR_MAX_COUNT + 63) / 64, 1, 1);

  std::array<VkBufferMemoryBarrier2, 2> barriers{};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = mCommands.handle;
  barriers[0].size = VK_WHOLE_SIZE;

  barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].buffer = mBisectors.handle;
  barriers[1].size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = barriers.size();
  dependencyInfo.pBufferMemoryBarriers = barriers.data();
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::Classify(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          mGenerateCmdsLayout, 0, 1, descriptorSets, 0,
                          nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mClassifyShader);

  vkCmdPushConstants(cmdBuffer, mGenerateCmdsLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT,
                     mGenCmdsPushConstantRange.offset,
                     mGenCmdsPushConstantRange.size, &mGenCmdsPushConstants);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);

  std::array<VkBufferMemoryBarrier2, 2> barriers{};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = mCommands.handle;
  barriers[0].size = VK_WHOLE_SIZE;

  barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].buffer = mBisectors.handle;
  barriers[1].size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = barriers.size();
  dependencyInfo.pBufferMemoryBarriers = barriers.data();
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::GenerateCmds(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          mGenerateCmdsLayout, 0, 1, descriptorSets, 0,
                          nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mGenerateCmdsShader);

  vkCmdPushConstants(cmdBuffer, mGenerateCmdsLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT,
                     mGenCmdsPushConstantRange.offset,
                     mGenCmdsPushConstantRange.size, &mGenCmdsPushConstants);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);

  std::array<VkBufferMemoryBarrier2, 3> barriers{};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = mBisectors.handle;
  barriers[0].size = VK_WHOLE_SIZE;

  barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].buffer = mCommands.handle;
  barriers[1].size = VK_WHOLE_SIZE;

  barriers[2].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[2].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[2].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[2].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[2].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[2].buffer = mAllocCounter.handle;
  barriers[2].size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = barriers.size();
  dependencyInfo.pBufferMemoryBarriers = barriers.data();
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::ValidateCmds(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0,
                          1, descriptorSets, 0, nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mValidateCmdsShader);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);

  std::array<VkBufferMemoryBarrier2, 1> barriers{};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = mCommands.handle;
  barriers[0].size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = barriers.size();
  dependencyInfo.pBufferMemoryBarriers = barriers.data();
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::ReserveBlocks(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0,
                          1, descriptorSets, 0, nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mReserveBlocksShader);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);

  std::array<VkBufferMemoryBarrier2, 3> barriers{};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = mBisectors.handle;
  barriers[0].size = VK_WHOLE_SIZE;

  barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].buffer = mCommands.handle;
  barriers[1].size = VK_WHOLE_SIZE;

  barriers[2].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[2].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[2].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[2].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[2].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[2].buffer = mAllocCounter.handle;
  barriers[2].size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = barriers.size();
  dependencyInfo.pBufferMemoryBarriers = barriers.data();
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::FillNewBlocks(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0,
                          1, descriptorSets, 0, nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mFillNewBlocksShader);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);

  VkBufferMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = mBisectors.handle;
  barrier.size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = 1;
  dependencyInfo.pBufferMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::UpdateNeighbours(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0,
                          1, descriptorSets, 0, nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mUpdateNeighboursShader);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);

  VkBufferMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = mBisectors.handle;
  barrier.size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = 1;
  dependencyInfo.pBufferMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::UpdateBitfield(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0,
                          1, descriptorSets, 0, nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mUpdateBitfieldShader);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);

  std::array<VkBufferMemoryBarrier2, 2> barriers{};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = mCbt.handle;
  barriers[0].size = VK_WHOLE_SIZE;

  barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[1].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[1].buffer = mBisectors.handle;
  barriers[1].size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = barriers.size();
  dependencyInfo.pBufferMemoryBarriers = barriers.data();
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::SumReduction(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          mSumReductionLayout, 0, 1, descriptorSets, 0,
                          nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mSumReductionShader);

  // Sum reduction
  for (int32_t depth = CBT_FIRST_REAL_LEVEL; depth >= 0; depth--)
  {
    SumReducPushConstants pushConstant{.depth = static_cast<uint32_t>(depth)};
    vkCmdPushConstants(cmdBuffer, mSumReductionLayout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       mSumReducPushConstantRange.offset,
                       mSumReducPushConstantRange.size, &pushConstant);

    uint32_t nodeCount = 1u << depth;
    uint32_t groupCount = (nodeCount + 63) / 64;
    vkCmdDispatch(cmdBuffer, groupCount, 1, 1);

    VkBufferMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = mCbt.handle;
    barrier.size = VK_WHOLE_SIZE;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.bufferMemoryBarrierCount = 1;
    dependencyInfo.pBufferMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
  }
}

void CbtBisection::CacheVertices(VkCommandBuffer cmdBuffer)
{
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          mGenerateCmdsLayout, 0, 1, descriptorSets, 0,
                          nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mCacheVertexShader);

  vkCmdPushConstants(cmdBuffer, mGenerateCmdsLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT,
                     mGenCmdsPushConstantRange.offset,
                     mGenCmdsPushConstantRange.size, &mGenCmdsPushConstants);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);

  std::array<VkBufferMemoryBarrier2, 1> barriers{};
  barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barriers[0].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barriers[0].dstAccessMask =
      VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barriers[0].buffer = mVertexCache.handle;
  barriers[0].size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = barriers.size();
  dependencyInfo.pBufferMemoryBarriers = barriers.data();
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::PlanetDisplacement(VkCommandBuffer cmdBuffer)
{
  // Fill indirect dispatch buffer
  VkDescriptorSet descriptorSets[] = {mDescriptorSet.Get()};
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0,
                          1, descriptorSets, 0, nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage,
                      &mIndirectDispatchVertexShader);

  vkCmdDispatch(cmdBuffer, 1, 1, 1);

  VkBufferMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
  barrier.dstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = mDispatchBuffer.handle;
  barrier.size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = 1;
  dependencyInfo.pBufferMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);

  // Displacement
  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          mGenerateCmdsLayout, 0, 1, descriptorSets, 0,
                          nullptr);

  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mPlanetDisplacementShader);

  vkCmdPushConstants(cmdBuffer, mGenerateCmdsLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT,
                     mGenCmdsPushConstantRange.offset,
                     mGenCmdsPushConstantRange.size, &mGenCmdsPushConstants);

  vkCmdDispatchIndirect(cmdBuffer, mDispatchBuffer.handle, 0);
}

void CbtBisection::VertShaderPipeBarrier(VkCommandBuffer cmdBuffer) noexcept
{
  VkBufferMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
  barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
  barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
  barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT |
                         VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
  barrier.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT |
                          VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.buffer = mVertexCache.handle;
  barrier.size = VK_WHOLE_SIZE;

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.bufferMemoryBarrierCount = 1;
  dependencyInfo.pBufferMemoryBarriers = &barrier;
  vkCmdPipelineBarrier2(cmdBuffer, &dependencyInfo);
}

void CbtBisection::Destroy()
{
  if (gContext.GetDevice() == VK_NULL_HANDLE)
    return;

  DestroyBuffer(mCbt);
  DestroyBuffer(mPointerCache);
  DestroyBuffer(mDispatchBuffer);
  DestroyBuffer(mBisectors);
  DestroyBuffer(mHalfedges);
  DestroyBuffer(mAllocCounter);
  DestroyBuffer(mCommands);
  DestroyBuffer(mVertexCache);
  DestroyBuffer(mGlobalUpdate);
  DestroyBuffer(mDebugBuffer);

  if (mIndirectDispatchPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(gContext.GetDevice(), mIndirectDispatchPool,
                            nullptr);
  }
  if (mIndirectDispatchSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(gContext.GetDevice(),
                                 mIndirectDispatchSetLayout, nullptr);
  }

  if (mDescriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(gContext.GetDevice(), mDescriptorPool, nullptr);
  }
  if (mDescriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(gContext.GetDevice(), mDescriptorSetLayout,
                                 nullptr);
  }

  PFN_vkDestroyShaderEXT vkDestroyShaderEXT{nullptr};
  if (gContext.GetInstance() != VK_NULL_HANDLE)
  {
    vkDestroyShaderEXT = (PFN_vkDestroyShaderEXT)vkGetInstanceProcAddr(
        gContext.GetInstance(), "vkDestroyShaderEXT");
  }

  std::array<VkShaderEXT, 14> shaders{mIndirectDispatchShader,
                                      mPointerCacheShader,
                                      mResetCommandsShader,
                                      mGenerateCmdsShader,
                                      mValidateCmdsShader,
                                      mReserveBlocksShader,
                                      mFillNewBlocksShader,
                                      mUpdateNeighboursShader,
                                      mUpdateBitfieldShader,
                                      mSumReductionShader,
                                      mCacheVertexShader,
                                      mClassifyShader,
                                      mIndirectDispatchVertexShader,
                                      mPlanetDisplacementShader};

  for (auto shader : shaders)
  {
    if (vkDestroyShaderEXT != nullptr && shader != VK_NULL_HANDLE)
      vkDestroyShaderEXT(gContext.GetDevice(), shader, nullptr);
  }
  if (mIndirectDispatchLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(gContext.GetDevice(), mIndirectDispatchLayout,
                            nullptr);
  if (mLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(gContext.GetDevice(), mLayout, nullptr);
  if (mSumReductionLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(gContext.GetDevice(), mSumReductionLayout, nullptr);
  if (mGenerateCmdsLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(gContext.GetDevice(), mGenerateCmdsLayout, nullptr);
}
