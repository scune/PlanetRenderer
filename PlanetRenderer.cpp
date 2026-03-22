#include "PlanetRenderer.hpp"

#include "Buffers.hpp"
#include "Context.hpp"
#include "DescriptorBuilder.hpp"
#include "GetVkFunc.hpp"
#include "Images.hpp"
#include "Libs.hpp"
#include "MeshGen.hpp"
#include "Noise.hpp"
#include "ResultCheck.hpp"
#include "Shader.hpp"
#include "Shaders/PlanetVertex.glsl.hpp"
#include "Swapchain.hpp"
#include "Textures.hpp"

void PlanetRenderer::Init()
{
  std::vector<Cbt::Halfedge> halfedges;
  // MeshGen::CubeHalfedges(halfedges, mVertices, 1.f);
  MeshGen::IsosahedronHalfedges(halfedges, mVertices);

  CreateBuffers();
  CreateDescriptor();
  CreateGraphicsShader();
  // CreateComputeShader();

  uint32_t maxSubdivision = 12;
  mCbtBisection.Init(maxSubdivision, halfedges, mVertexBuffer, 10000.f);

  mCam.SetPos(glm::vec3(0.f, -11200.f, 0.f));
  mCam.SetRot(glm::vec3(0.f, 1.f, 0.f));
  mCam.SetSpeed(1000.f);
}

void PlanetRenderer::CreateBuffers()
{
  const VkMemoryPropertyFlags memProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  mVertexBuffer.size = sizeof(glm::vec4) * 32768;
  mVertexBuffer.usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                             VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  mVertexBuffer.memProperties = memProps;
  IfNThrow(CreateBuffer(mVertexBuffer), "Failed to create vertex buffer!");

  mIndexBuffer.size = sizeof(uint32_t) * 32768;
  mIndexBuffer.usageFlags =
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  mIndexBuffer.memProperties = memProps;
  IfNThrow(CreateBuffer(mIndexBuffer), "Failed to create index buffer!");

  if (mVertices.size() > 0)
  {
    IfNThrow(BufferCopyToHost(mVertexBuffer, mVertices.data(),
                              VecByteSize(mVertices)),
             "Failed to copy data to vertex buffer!");
  }
  if (mIndices.size() > 0)
  {
    IfNThrow(
        BufferCopyToHost(mIndexBuffer, mIndices.data(), VecByteSize(mIndices)),
        "Failed to copy data to index buffer!");
  }

  mSceneData.size = sizeof(SceneData_t);
  mSceneData.usageFlags =
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  mSceneData.memProperties = memProps;
  IfNThrow(CreateBuffer(mSceneData), "Failed to create scene data buffer!");

  auto txtLoadRes = Textures::LoadFromFile("aerial_rocks_04_diff_1k.jpg");
  IfNThrow(txtLoadRes.has_value(), "Failed to load texture!");
  mTexture = txtLoadRes.value();

  auto txt2LoadRes = Textures::LoadFromFile("brown_mud_leaves_01_diff_1k.jpg");
  IfNThrow(txt2LoadRes.has_value(), "Failed to load texture!");
  mTexture2 = txt2LoadRes.value();
  /*mTexture.extent = VkExtent2D{1024, 1024};
  mTexture.format = VK_FORMAT_R8G8B8A8_UNORM;
  mTexture.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
  CreateImage(mTexture, VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  CreateSampler(mTexture, VK_FALSE, VK_TRUE);

  mTexture2.extent = VkExtent2D{1024, 1024};
  mTexture2.format = VK_FORMAT_R8G8B8A8_UNORM;
  mTexture2.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
  CreateImage(mTexture2, VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  CreateSampler(mTexture2, VK_FALSE, VK_TRUE);*/

  mDepthImage.extent = gSwapchain.GetExtent();
  mDepthImage.format = gContext.FindSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
          VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
  mDepthImage.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
  CreateImage(mDepthImage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  gContext.ResetAndBeginCmdBufferOneTime();
  ImageInitTransitionLayoutCmd(
      gContext.GetCmdBufferOneTime(), mDepthImage,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  gContext.EndCmdBufferOneTime();
  gContext.OneTimeSubmit();
}

void PlanetRenderer::CreateDescriptor()
{
  DescriptorBuilder<3> builder;
  builder.AddUBO(mSceneData,
                 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
  builder.AddCombImgSampler(mTexture, VK_SHADER_STAGE_FRAGMENT_BIT);
  builder.AddCombImgSampler(mTexture2, VK_SHADER_STAGE_FRAGMENT_BIT);

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

void PlanetRenderer::CreateComputeShader()
{
  std::array<VkDescriptorSetLayout, 1> setLayouts{
      gSwapchain.GetDescriptorSetLayout()};

  {
    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = setLayouts.size();
    createInfo.pSetLayouts = setLayouts.data();

    IfNThrow(vkCreatePipelineLayout(gContext.GetDevice(), &createInfo, nullptr,
                                    &mLayout),
             "Failed to create pipeline layout!");
  }

  std::vector<uint32_t> shaderCode;
  Shader::LoadSPV("PlanetShader.comp.spv", shaderCode);

  VkShaderCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
  createInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  createInfo.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
  createInfo.codeSize = VecByteSize(shaderCode);
  createInfo.pCode = shaderCode.data();
  createInfo.setLayoutCount = setLayouts.size();
  createInfo.pSetLayouts = setLayouts.data();
  createInfo.pName = "main";

  GetVkFuncAuto(gContext.GetInstance(), vkCreateShadersEXT);

  IfNThrow(vkCreateShadersEXT(gContext.GetDevice(), 1, &createInfo, nullptr,
                              &mShader[0]),
           "Failed to create shader!");

  vkCmdBindShadersEXT = GetVkFunc(gContext.GetInstance(), vkCmdBindShadersEXT);
}

void PlanetRenderer::CreateGraphicsShader()
{
  std::array<VkDescriptorSetLayout, 1> setLayouts{mDescriptorSetLayout};

  {
    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = setLayouts.size();
    createInfo.pSetLayouts = setLayouts.data();

    IfNThrow(vkCreatePipelineLayout(gContext.GetDevice(), &createInfo, nullptr,
                                    &mLayout),
             "Failed to create pipeline layout!");
  }

  std::vector<uint32_t> vertCode;
  Shader::LoadSPV("PlanetShader.vert.spv", vertCode);

  std::vector<uint32_t> fragCode;
  Shader::LoadSPV("PlanetShader.frag.spv", fragCode);

  VkShaderCreateInfoEXT shaderCreateInfos[2] = {
      {.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
       .pNext = nullptr,
       .flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .nextStage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
       .codeSize = VecByteSize(vertCode),
       .pCode = vertCode.data(),
       .pName = "main",
       .setLayoutCount = setLayouts.size(),
       .pSetLayouts = setLayouts.data(),
       .pushConstantRangeCount = 0,
       .pPushConstantRanges = nullptr,
       .pSpecializationInfo = nullptr},
      {.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
       .pNext = nullptr,
       .flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .nextStage = 0,
       .codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT,
       .codeSize = VecByteSize(fragCode),
       .pCode = fragCode.data(),
       .pName = "main",
       .setLayoutCount = setLayouts.size(),
       .pSetLayouts = setLayouts.data(),
       .pushConstantRangeCount = 0,
       .pPushConstantRanges = nullptr,
       .pSpecializationInfo = nullptr}};

  GetVkFuncAuto(gContext.GetInstance(), vkCreateShadersEXT);

  IfNThrow(vkCreateShadersEXT(gContext.GetDevice(), 2, shaderCreateInfos,
                              nullptr, mShader.data()),
           "Failed to create shader!");

  vkCmdBindShadersEXT = GetVkFunc(gContext.GetInstance(), vkCmdBindShadersEXT);
  vkCmdSetPolygonModeEXT =
      GetVkFunc(gContext.GetInstance(), vkCmdSetPolygonModeEXT);
  vkCmdSetRasterizationSamplesEXT =
      GetVkFunc(gContext.GetInstance(), vkCmdSetRasterizationSamplesEXT);
  vkCmdSetSampleMaskEXT =
      GetVkFunc(gContext.GetInstance(), vkCmdSetSampleMaskEXT);
  vkCmdSetAlphaToCoverageEnableEXT =
      GetVkFunc(gContext.GetInstance(), vkCmdSetAlphaToCoverageEnableEXT);
  vkCmdSetColorBlendEnableEXT =
      GetVkFunc(gContext.GetInstance(), vkCmdSetColorBlendEnableEXT);
  vkCmdSetColorWriteMaskEXT =
      GetVkFunc(gContext.GetInstance(), vkCmdSetColorWriteMaskEXT);
  vkCmdSetVertexInputEXT =
      GetVkFunc(gContext.GetInstance(), vkCmdSetVertexInputEXT);
}

void PlanetRenderer::Update()
{
  mCam.Update();
  /*
  if (gContext.GetFrameCounter() % 16 == 0)
  {
    CbtLodTest(mVertices, mIndices, mCbtData, 14, mCam.GetPos(), 100.f);

    IfNThrow(BufferCopyToHost(mVertexBuffer, mVertices.data(),
                              VecByteSize(mVertices)),
             "Failed to copy data to vertex buffer!");
    IfNThrow(
        BufferCopyToHost(mIndexBuffer, mIndices.data(), VecByteSize(mIndices)),
        "Failed to copy data to index buffer!");
  }*/

  // COUT_VEC3(mCam.GetPos());
  // COUT_VEC3(mCam.GetRot());

  SceneData_t data{};
  data.camMatrix = mCam.GetMatrix();
  IfNThrow(BufferCopyToHost(mSceneData, &data),
           "Failed to update scene data buffer!");

  mCbtBisection.Update(mCam.GetPos(), mCam.GetMatrix(), mCam.GetRot());
}

void PlanetRenderer::DrawCompute(VkCommandBuffer cmdBuffer,
                                 uint32_t swapchainImgIdx) noexcept
{
  std::array<VkDescriptorSet, 1> descriptorSets{
      gSwapchain.GetDescriptorSet(gSwapchain.GetFrameImgIndex())};

  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0,
                          descriptorSets.size(), descriptorSets.data(), 0,
                          nullptr);

  VkShaderStageFlagBits shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
  vkCmdBindShadersEXT(cmdBuffer, 1, &shaderStage, &mShader[0]);

  gSwapchain.TransitionForCompute(cmdBuffer, swapchainImgIdx);

  glm::uvec2 dispatchCount =
      gContext.GetDispatchCount2D(gSwapchain.GetExtent());
  vkCmdDispatch(cmdBuffer, dispatchCount.x, dispatchCount.y, 1);

  gSwapchain.TransitionAfterCompute(cmdBuffer, swapchainImgIdx);
}

void PlanetRenderer::DrawGraphics(VkCommandBuffer cmdBuffer,
                                  uint32_t swapchainImgIdx) noexcept
{
  // if (gContext.GetFrameCounter() % 32 == 0)
  {
    mCbtBisection.RecordCmds(cmdBuffer);
    mCbtBisection.VertShaderPipeBarrier(cmdBuffer);
  }

  // Transition swapchain
  gSwapchain.TransitionForGraphics(cmdBuffer, swapchainImgIdx);

  // Descriptor
  std::array<VkDescriptorSet, 1> descriptorSets{mDescriptorSet.Get()};

  vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout,
                          0, descriptorSets.size(), descriptorSets.data(), 0,
                          nullptr);

  // Renderpass
  VkRenderingAttachmentInfo colorAttachmentInfo{};
  colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachmentInfo.imageView =
      gSwapchain.GetViews()[gSwapchain.GetFrameImgIndex()];
  colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  VkRenderingAttachmentInfo depthAttachmentInfo{};
  depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depthAttachmentInfo.imageView = mDepthImage.view;
  depthAttachmentInfo.imageLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachmentInfo.clearValue.depthStencil = {.depth = 1.f, .stencil = 0};

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea = {.offset = {}, .extent = mDepthImage.extent};
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachmentInfo;
  renderingInfo.pDepthAttachment = &depthAttachmentInfo;

  vkCmdBeginRendering(cmdBuffer, &renderingInfo);

  // Shaders
  const VkShaderStageFlagBits stages[2] = {VK_SHADER_STAGE_VERTEX_BIT,
                                           VK_SHADER_STAGE_FRAGMENT_BIT};
  vkCmdBindShadersEXT(cmdBuffer, 2, stages, mShader.data());

  VkViewport viewport{};
  viewport.width = mDepthImage.extent.width;
  viewport.height = mDepthImage.extent.height;
  viewport.maxDepth = 1.f;
  vkCmdSetViewportWithCount(cmdBuffer, 1, &viewport);

  VkRect2D scissor{};
  scissor.extent = mDepthImage.extent;
  vkCmdSetScissorWithCount(cmdBuffer, 1, &scissor);

  vkCmdSetRasterizerDiscardEnable(cmdBuffer, VK_FALSE);

  vkCmdSetPrimitiveRestartEnable(cmdBuffer, VK_FALSE);
  vkCmdSetPrimitiveTopology(cmdBuffer, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  vkCmdSetPolygonModeEXT(cmdBuffer, VK_POLYGON_MODE_FILL);
  // vkCmdSetPolygonModeEXT(cmdBuffer, VK_POLYGON_MODE_LINE);
  vkCmdSetDepthBiasEnable(cmdBuffer, VK_FALSE);

  // vkCmdSetCullMode(cmdBuffer, VK_CULL_MODE_BACK_BIT);
  vkCmdSetCullMode(cmdBuffer, VK_CULL_MODE_NONE);
  vkCmdSetFrontFace(cmdBuffer, VK_FRONT_FACE_COUNTER_CLOCKWISE);

  vkCmdSetLineWidth(cmdBuffer, 1.f);

  vkCmdSetDepthTestEnable(cmdBuffer, VK_TRUE);
  vkCmdSetDepthWriteEnable(cmdBuffer, VK_TRUE);
  vkCmdSetDepthCompareOp(cmdBuffer, VK_COMPARE_OP_LESS);
  vkCmdSetStencilTestEnable(cmdBuffer, VK_FALSE);

  vkCmdSetRasterizationSamplesEXT(cmdBuffer, VK_SAMPLE_COUNT_1_BIT);
  VkSampleMask sampleMask = 0xFFFFFFFF;
  vkCmdSetSampleMaskEXT(cmdBuffer, VK_SAMPLE_COUNT_1_BIT, &sampleMask);
  vkCmdSetAlphaToCoverageEnableEXT(cmdBuffer, VK_FALSE);

  VkBool32 colorBlendEnable = VK_FALSE;
  vkCmdSetColorBlendEnableEXT(cmdBuffer, 0, 1, &colorBlendEnable);
  VkColorComponentFlags colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  vkCmdSetColorWriteMaskEXT(cmdBuffer, 0, 1, &colorWriteMask);

  VkVertexInputBindingDescription2EXT vertexInputBinding{};
  vertexInputBinding.sType =
      VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
  vertexInputBinding.binding = 0;
  vertexInputBinding.stride = sizeof(Cbt::PlanetVertex);
  vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertexInputBinding.divisor = 1;

  std::array<VkVertexInputAttributeDescription2EXT, 2> vertexInputAttributes{};
  vertexInputAttributes[0].sType =
      VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
  vertexInputAttributes[0].location = 0;
  vertexInputAttributes[0].binding = 0;
  vertexInputAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  vertexInputAttributes[0].offset = 0;

  vertexInputAttributes[1].sType =
      VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
  vertexInputAttributes[1].location = 1;
  vertexInputAttributes[1].binding = 0;
  vertexInputAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  vertexInputAttributes[1].offset = offsetof(Cbt::PlanetVertex, normal);
  vkCmdSetVertexInputEXT(cmdBuffer, 1, &vertexInputBinding,
                         vertexInputAttributes.size(),
                         vertexInputAttributes.data());

  // Geometry buffer
  const VkDeviceSize vertexOffset[1] = {sizeof(VkDrawIndirectCommand)};
  // vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mVertexBuffer.handle,
  // vertexOffset);
  vkCmdBindVertexBuffers(cmdBuffer, 0, 1,
                         &mCbtBisection.GetVertexCacheBuffer().handle,
                         vertexOffset);

  // vkCmdBindIndexBuffer(cmdBuffer, mIndexBuffer.handle, 0,
  // VK_INDEX_TYPE_UINT32);

  // Draw
  vkCmdDrawIndirect(cmdBuffer, mCbtBisection.GetVertexCacheBuffer().handle, 0,
                    1, sizeof(VkDrawIndirectCommand));

  // Transition swapchain
  vkCmdEndRendering(cmdBuffer);

  gSwapchain.TransitionAfterGraphics(cmdBuffer, swapchainImgIdx);
};

void PlanetRenderer::Destroy() noexcept
{
  if (gContext.GetDevice() == VK_NULL_HANDLE)
    return;

  mCbtBisection.Destroy();

  DestroyBuffer(mVertexBuffer);
  DestroyBuffer(mIndexBuffer);
  DestroyBuffer(mSceneData);
  DestroyImage(mTexture);
  DestroyImage(mTexture2);
  DestroyImage(mDepthImage);

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

  for (auto shader : mShader)
  {
    if (vkDestroyShaderEXT != nullptr && shader != VK_NULL_HANDLE)
      vkDestroyShaderEXT(gContext.GetDevice(), shader, nullptr);
  }
  if (mLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(gContext.GetDevice(), mLayout, nullptr);
}
