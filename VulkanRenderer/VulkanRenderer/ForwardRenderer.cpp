#pragma once
#include "VulkanUtils.h"
#include "GBuffer.h"
#include "VulkanContext.h"
#include "Scene.h"
#include <iostream>
#include "Vertex.h"
#include "DepthImage.h"
#include "ForwardRenderer.h"


ForwardRenderer::ForwardRenderer(VulkanContext* context, Scene* scene)
    :context(context), scene(scene) {}
ForwardRenderer::~ForwardRenderer() {}



void ForwardRenderer::createDepthPass()
{
    VkAttachmentDescription depthAttachment = DepthImage::getDepthAttachmentDepthPass(context->m_physicalDevice);

    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 0;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 0;
    subpassDescription.pColorAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // ToDo civ
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // ToDo civ
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dependencyFlags = 0;


    std::vector<VkAttachmentDescription> attachments;
    attachments.push_back(depthAttachment);

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;


    VkResult result = vkCreateRenderPass(context->m_device, &renderPassCreateInfo, nullptr, &depthPass);
    ASSERT_VULKAN(result);
}


void ForwardRenderer::createRenderPass() {
    VkAttachmentDescription attachmentDescription;
    attachmentDescription.flags = 0;
    attachmentDescription.format = context->m_colorFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


    VkAttachmentReference attachmentReference;
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = DepthImage::getDepthAttachmentRenderPass(context->m_physicalDevice);

    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;




    VkSubpassDescription subpassDescription;
    subpassDescription.flags = 0;
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &attachmentReference;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;

    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency.dependencyFlags = 0;


    std::vector<VkAttachmentDescription> attachments;
    attachments.push_back(attachmentDescription);
    attachments.push_back(depthAttachment);

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;


    VkResult result = vkCreateRenderPass(context->m_device, &renderPassCreateInfo, nullptr, &renderPass);
    ASSERT_VULKAN(result);
}


void ForwardRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding;
    samplerDescriptorSetLayoutBinding.binding = 1;
    samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    samplerDescriptorSetLayoutBinding.descriptorCount = scene->m_textureCount; //1
    samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerDescriptorSetLayoutBinding.pImmutableSamplers = 0;

    VkDescriptorSetLayoutBinding materialDescriptorSetLayoutBinding;
    materialDescriptorSetLayoutBinding.binding = 2;
    materialDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    materialDescriptorSetLayoutBinding.descriptorCount = 1;
    materialDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding decalDescriptorSetLayoutBinding;
    decalDescriptorSetLayoutBinding.binding = 3;
    decalDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    decalDescriptorSetLayoutBinding.descriptorCount = 1;
    decalDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    decalDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding accelerationStructureDescriptorSetLayoutBinding;
    accelerationStructureDescriptorSetLayoutBinding.binding = 4;
    accelerationStructureDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelerationStructureDescriptorSetLayoutBinding.descriptorCount = 1;
    accelerationStructureDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    accelerationStructureDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding accelerationStructureSceneDescriptorSetLayoutBinding;
    accelerationStructureSceneDescriptorSetLayoutBinding.binding = 5;
    accelerationStructureSceneDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelerationStructureSceneDescriptorSetLayoutBinding.descriptorCount = 1;
    accelerationStructureSceneDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    accelerationStructureSceneDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;


    VkDescriptorSetLayoutBinding pointLightDescriptorSetLayoutBinding;
    pointLightDescriptorSetLayoutBinding.binding = 6;
    pointLightDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pointLightDescriptorSetLayoutBinding.descriptorCount = 1;
    pointLightDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pointLightDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding accelerationStructurePointLightDescriptorSetLayoutBinding;
    accelerationStructurePointLightDescriptorSetLayoutBinding.binding = 7;
    accelerationStructurePointLightDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelerationStructurePointLightDescriptorSetLayoutBinding.descriptorCount = 1;
    accelerationStructurePointLightDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    accelerationStructurePointLightDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> descriptorSets;
    descriptorSets.push_back(descriptorSetLayoutBinding);
    descriptorSets.push_back(samplerDescriptorSetLayoutBinding);
    descriptorSets.push_back(materialDescriptorSetLayoutBinding);
    descriptorSets.push_back(decalDescriptorSetLayoutBinding);
    descriptorSets.push_back(accelerationStructureDescriptorSetLayoutBinding);
    descriptorSets.push_back(accelerationStructureSceneDescriptorSetLayoutBinding);
    descriptorSets.push_back(pointLightDescriptorSetLayoutBinding);
    descriptorSets.push_back(accelerationStructurePointLightDescriptorSetLayoutBinding);


    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = descriptorSets.size();
    descriptorSetLayoutCreateInfo.pBindings = descriptorSets.data();


    VkResult result = vkCreateDescriptorSetLayout(context->m_device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
    ASSERT_VULKAN(result);


}

void ForwardRenderer::compileShader() {

    std::string cmdShaderCompile = "C:\\VulkanSDK\\1.2.141.2\\Bin\\glslangValidator.exe";

    std::string cmdVertexShaderCompile = cmdShaderCompile
        + " -V -o shader/vert.spv shader/shader.vert";
    //+ " -V -o shader/vert.spv shader/shader_deferred_shading_pass.glsl.vert";

    std::string cmdFragmentShaderCompile = cmdShaderCompile;
    cmdFragmentShaderCompile
        .append(" -DTEXTURES=").append(std::to_string(scene->m_textureCount))
        .append(" -DMATERIALS=").append(std::to_string(scene->m_materialCount))
        .append(" -DDECALS=").append(std::to_string(scene->m_decalCount + 1))
        .append(" -DPOINT_LIGHTS=").append(std::to_string(scene->m_pointLightCount))
        .append(createShaderFeatureFlagsComlileArguments(context->shaderFeatureFlags))
        .append(" -V -o shader/frag.spv shader/shader.frag");
    //.append(" -V -o shader/frag.spv shader/shader_deferred_shading_pass.glsl.frag");

    std::string cmdDepthPassVertexShaderCompile = cmdShaderCompile
        + " -V -o shader/shader_depth_pass_vert.spv shader/shader_depth_pass.glsl.vert";
    std::string cmdDepthPassFragmentShaderCompile = cmdShaderCompile;
    cmdDepthPassFragmentShaderCompile
        .append(" -DTEXTURES=").append(std::to_string(scene->m_textureCount))
        .append(" -DMATERIALS=").append(std::to_string(scene->m_materialCount))
        .append(" -V -o shader/shader_depth_pass_frag.spv shader/shader_depth_pass.glsl.frag");

    std::cout << cmdVertexShaderCompile << std::endl;
    std::cout << cmdFragmentShaderCompile << std::endl;
    std::cout << cmdDepthPassVertexShaderCompile << std::endl;
    std::cout << cmdDepthPassFragmentShaderCompile << std::endl;

    system(cmdVertexShaderCompile.c_str());
    system(cmdFragmentShaderCompile.c_str());

    system(cmdDepthPassVertexShaderCompile.c_str());
    system(cmdDepthPassFragmentShaderCompile.c_str());

    auto shaderCodeVert = readFile("shader/vert.spv");
    auto shaderCodeFrag = readFile("shader/frag.spv");

    auto shaderCodeDepthPassVert = readFile("shader/shader_depth_pass_vert.spv");
    auto shaderCodeDepthPassFrag = readFile("shader/shader_depth_pass_frag.spv");

    createShaderModule(context->m_device, shaderCodeVert, &shaderModuleVert);
    createShaderModule(context->m_device, shaderCodeFrag, &shaderModuleFrag);

    createShaderModule(context->m_device, shaderCodeDepthPassVert, &shaderModuleDepthPassVert);
    createShaderModule(context->m_device, shaderCodeDepthPassFrag, &shaderModuleDepthPassFrag);
}



void ForwardRenderer::createDepthPassPipeline()
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    shaderStageCreateInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoVert.pNext = nullptr;
    shaderStageCreateInfoVert.flags = 0;
    shaderStageCreateInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfoVert.module = shaderModuleDepthPassVert;
    shaderStageCreateInfoVert.pName = "main";
    shaderStageCreateInfoVert.pSpecializationInfo = nullptr;



    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    shaderStageCreateInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoFrag.pNext = nullptr;
    shaderStageCreateInfoFrag.flags = 0;
    shaderStageCreateInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfoFrag.module = shaderModuleDepthPassFrag;
    shaderStageCreateInfoFrag.pName = "main";
    shaderStageCreateInfoFrag.pSpecializationInfo = nullptr;



    VkPipelineShaderStageCreateInfo shaderStages[] = {
        shaderStageCreateInfoVert,
        shaderStageCreateInfoFrag
    };


    auto vertexBindingDescription = Vertex::getBindingDescription();
    auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.pNext = nullptr;
    vertexInputCreateInfo.flags = 0;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();




    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo;
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.pNext = nullptr;
    inputAssemblyCreateInfo.flags = 0;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = context->m_width;
    viewport.height = context->m_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = { context->m_width, context->m_height };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.pNext = nullptr;
    viewportStateCreateInfo.flags = 0;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;


    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo;
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.pNext = nullptr;
    rasterizationCreateInfo.flags = 0;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationCreateInfo.lineWidth = 1.0f;


    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo;
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.pNext = nullptr;
    multisampleCreateInfo.flags = 0;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.pSampleMask = nullptr;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;


    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = DepthImage::getDepthStencilStateCreateInfoOpaqueDepthPass();

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;

    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pNext = nullptr;
    dynamicStateCreateInfo.flags = 0;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;


    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(context->m_device, &pipelineLayoutCreateInfo, nullptr, &depthPassPipelineLayout);
    ASSERT_VULKAN(result);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.flags = 0; //VK_PIPELINE_CREATE_DERIVATIVE_BIT? TODO civ
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pTessellationState = nullptr;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = nullptr;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = depthPassPipelineLayout;
    pipelineCreateInfo.renderPass = depthPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = nullptr; // Derivation? TODO civ
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(context->m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &depthPassPipeline);
    ASSERT_VULKAN(result);
}

void ForwardRenderer::createPipeline()
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    shaderStageCreateInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoVert.pNext = nullptr;
    shaderStageCreateInfoVert.flags = 0;
    shaderStageCreateInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfoVert.module = shaderModuleVert; //TODO fix
    shaderStageCreateInfoVert.pName = "main";
    shaderStageCreateInfoVert.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    shaderStageCreateInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoFrag.pNext = nullptr;
    shaderStageCreateInfoFrag.flags = 0;
    shaderStageCreateInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfoFrag.module = shaderModuleFrag; //TODO fix
    shaderStageCreateInfoFrag.pName = "main";
    shaderStageCreateInfoFrag.pSpecializationInfo = nullptr;


    VkPipelineShaderStageCreateInfo shaderStages[] = {
        shaderStageCreateInfoVert,
        shaderStageCreateInfoFrag
    };


    auto vertexBindingDescription = Vertex::getBindingDescription();
    auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.pNext = nullptr;
    vertexInputCreateInfo.flags = 0;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();




    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo;
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.pNext = nullptr;
    inputAssemblyCreateInfo.flags = 0;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = context->m_width;
    viewport.height = context->m_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;



    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = { context->m_width, context->m_height };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.pNext = nullptr;
    viewportStateCreateInfo.flags = 0;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;


    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo;

    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.pNext = nullptr;
    rasterizationCreateInfo.flags = 0;
    rasterizationCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationCreateInfo.depthBiasClamp = 0.0f;
    rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationCreateInfo.lineWidth = 1.0f;


    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo;
    multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleCreateInfo.pNext = nullptr;
    multisampleCreateInfo.flags = 0;
    multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleCreateInfo.minSampleShading = 1.0f;
    multisampleCreateInfo.pSampleMask = nullptr;
    multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleCreateInfo.alphaToOneEnable = VK_FALSE;


    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = DepthImage::getDepthStencilStateCreateInfoOpaqueRenderPass();


    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.pNext = nullptr;
    colorBlendCreateInfo.flags = 0;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
    colorBlendCreateInfo.blendConstants[0] = 0.0f;
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;



    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;

    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pNext = nullptr;
    dynamicStateCreateInfo.flags = 0;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;


    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = nullptr;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout; // TODO fix
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(context->m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
    ASSERT_VULKAN(result);

    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = nullptr;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pTessellationState = nullptr;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(context->m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
    ASSERT_VULKAN(result);
}


void ForwardRenderer::createFramebuffers() {

    framebuffers = new VkFramebuffer[context->m_amountOfImagesInSwapchain];

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        std::vector<VkImageView> attachmentViews;
        attachmentViews.push_back(context->m_imageViews[i]);
        attachmentViews.push_back(depthImage.getImageView());


        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = attachmentViews.size();
        framebufferCreateInfo.pAttachments = attachmentViews.data();
        framebufferCreateInfo.width = context->m_width;
        framebufferCreateInfo.height = context->m_height;
        framebufferCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(context->m_device, &framebufferCreateInfo, nullptr, &(framebuffers[i]));
        ASSERT_VULKAN(result);
    }
}


void ForwardRenderer::createFramebuffersDepthPass()
{
    std::vector<VkImageView> attachmentViews;
    attachmentViews.push_back(depthImage.getImageView());

    VkFramebufferCreateInfo framebufferCreateInfo;
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.pNext = nullptr;
    framebufferCreateInfo.flags = 0;
    framebufferCreateInfo.renderPass = depthPass;
    framebufferCreateInfo.attachmentCount = attachmentViews.size();
    framebufferCreateInfo.pAttachments = attachmentViews.data();
    framebufferCreateInfo.width = context->m_width;
    framebufferCreateInfo.height = context->m_height;
    framebufferCreateInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(context->m_device, &framebufferCreateInfo, nullptr, &depthPassFramebuffer);
    ASSERT_VULKAN(result);
}





void ForwardRenderer::createDepthImage()
{
    depthImage.create(context->m_device, context->m_physicalDevice, context->m_commandPool, context->m_queue, context->m_width, context->m_height);
}



void ForwardRenderer::createCommandBuffers() {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = context->m_commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = context->m_amountOfImagesInSwapchain;

    commandBuffers = new VkCommandBuffer[context->m_amountOfImagesInSwapchain];
    VkResult result = vkAllocateCommandBuffers(context->m_device, &commandBufferAllocateInfo, commandBuffers);
    ASSERT_VULKAN(result);
}





void ForwardRenderer::createDepthPassCommandBuffers() {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandPool = context->m_commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = context->m_amountOfImagesInSwapchain;
    commandBuffersDepthPass = new VkCommandBuffer[context->m_amountOfImagesInSwapchain];
    VkResult result = vkAllocateCommandBuffers(context->m_device, &commandBufferAllocateInfo, commandBuffersDepthPass);
    ASSERT_VULKAN(result);
}

// Create descriptor set per swapchain image
void ForwardRenderer::createDescriptorSets() {

    std::vector<VkDescriptorSetLayout> layouts(context->m_amountOfImagesInSwapchain, descriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = context->m_descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = context->m_amountOfImagesInSwapchain;
    descriptorSetAllocateInfo.pSetLayouts = layouts.data();

    descriptorSets = new VkDescriptorSet[context->m_amountOfImagesInSwapchain];

    VkResult result = vkAllocateDescriptorSets(context->m_device, &descriptorSetAllocateInfo, descriptorSets);
    ASSERT_VULKAN(result);

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        VkDescriptorBufferInfo descriptorBufferInfo[4];
        descriptorBufferInfo[0].buffer = context->m_uniformBuffers[0];
        descriptorBufferInfo[0].offset = getUniformBufferOffset(sizeof(context->m_perFrameConstants), i, context->m_deviceProperties);
        descriptorBufferInfo[0].range = sizeof(context->m_perFrameConstants);

        descriptorBufferInfo[1].buffer = context->m_uniformBuffers[1];
        descriptorBufferInfo[1].offset = 0;
        descriptorBufferInfo[1].range = VK_WHOLE_SIZE;

        descriptorBufferInfo[2].buffer = context->m_uniformBuffers[2];
        descriptorBufferInfo[2].offset = 0;
        descriptorBufferInfo[2].range = VK_WHOLE_SIZE;

        descriptorBufferInfo[3].buffer = context->m_uniformBuffers[3];
        descriptorBufferInfo[3].offset = 0;
        descriptorBufferInfo[3].range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet descriptorWrite;
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.pNext = nullptr;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pBufferInfo = &(descriptorBufferInfo[0]);
        descriptorWrite.pTexelBufferView = nullptr;

        VkWriteDescriptorSet materialDescriptorWrite;
        materialDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        materialDescriptorWrite.pNext = nullptr;
        materialDescriptorWrite.dstSet = descriptorSets[i];
        materialDescriptorWrite.dstBinding = 2;
        materialDescriptorWrite.dstArrayElement = 0;
        materialDescriptorWrite.descriptorCount = 1;
        materialDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        materialDescriptorWrite.pImageInfo = nullptr;
        materialDescriptorWrite.pBufferInfo = &(descriptorBufferInfo[1]);
        materialDescriptorWrite.pTexelBufferView = nullptr;

        VkWriteDescriptorSet decalDescriptorWrite;
        decalDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        decalDescriptorWrite.pNext = nullptr;
        decalDescriptorWrite.dstSet = descriptorSets[i];
        decalDescriptorWrite.dstBinding = 3;
        decalDescriptorWrite.dstArrayElement = 0;
        decalDescriptorWrite.descriptorCount = 1;
        decalDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        decalDescriptorWrite.pImageInfo = nullptr;
        decalDescriptorWrite.pBufferInfo = &(descriptorBufferInfo[2]);
        decalDescriptorWrite.pTexelBufferView = nullptr;

        VkWriteDescriptorSet pointLightDescriptorWrite;
        pointLightDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        pointLightDescriptorWrite.pNext = nullptr;
        pointLightDescriptorWrite.dstSet = descriptorSets[i];
        pointLightDescriptorWrite.dstBinding = 6;
        pointLightDescriptorWrite.dstArrayElement = 0;
        pointLightDescriptorWrite.descriptorCount = 1;
        //pointLightDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pointLightDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pointLightDescriptorWrite.pImageInfo = nullptr;
        pointLightDescriptorWrite.pBufferInfo = &(descriptorBufferInfo[3]);
        pointLightDescriptorWrite.pTexelBufferView = nullptr;



        VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureDescriptorWrite;
        accelerationStructureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureDescriptorWrite.pNext = nullptr;
        accelerationStructureDescriptorWrite.accelerationStructureCount = 1;
        accelerationStructureDescriptorWrite.pAccelerationStructures = &(scene->m_decal_as.topLevelAs.as);


        VkWriteDescriptorSet accelerationStructureWrite;
        accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        accelerationStructureWrite.pNext = &accelerationStructureDescriptorWrite;
        accelerationStructureWrite.dstSet = descriptorSets[i];
        accelerationStructureWrite.dstBinding = 4;
        accelerationStructureWrite.dstArrayElement = 0;
        accelerationStructureWrite.descriptorCount = 1;
        accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureWrite.pImageInfo = nullptr;
        accelerationStructureWrite.pBufferInfo = nullptr;
        accelerationStructureWrite.pTexelBufferView = nullptr;





        VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureSceneDescriptorWrite;
        accelerationStructureSceneDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureSceneDescriptorWrite.pNext = nullptr;
        accelerationStructureSceneDescriptorWrite.accelerationStructureCount = 1;
        accelerationStructureSceneDescriptorWrite.pAccelerationStructures = &(scene->m_scene_as.topLevelAs.as);


        VkWriteDescriptorSet accelerationStructureSceneWrite;
        accelerationStructureSceneWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        accelerationStructureSceneWrite.pNext = &accelerationStructureSceneDescriptorWrite;
        accelerationStructureSceneWrite.dstSet = descriptorSets[i];
        accelerationStructureSceneWrite.dstBinding = 5;
        accelerationStructureSceneWrite.dstArrayElement = 0;
        accelerationStructureSceneWrite.descriptorCount = 1;
        accelerationStructureSceneWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureSceneWrite.pImageInfo = nullptr;
        accelerationStructureSceneWrite.pBufferInfo = nullptr;
        accelerationStructureSceneWrite.pTexelBufferView = nullptr;


        VkWriteDescriptorSetAccelerationStructureKHR accelerationStructurePointLightDescriptorWrite;
        accelerationStructurePointLightDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        accelerationStructurePointLightDescriptorWrite.pNext = nullptr;
        accelerationStructurePointLightDescriptorWrite.accelerationStructureCount = 1;
        accelerationStructurePointLightDescriptorWrite.pAccelerationStructures = &(scene->m_point_light_as.topLevelAs.as);


        VkWriteDescriptorSet accelerationStructurePointLightWrite;
        accelerationStructurePointLightWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        accelerationStructurePointLightWrite.pNext = &accelerationStructurePointLightDescriptorWrite;
        accelerationStructurePointLightWrite.dstSet = descriptorSets[i];
        accelerationStructurePointLightWrite.dstBinding = 7;
        accelerationStructurePointLightWrite.dstArrayElement = 0;
        accelerationStructurePointLightWrite.descriptorCount = 1;
        accelerationStructurePointLightWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructurePointLightWrite.pImageInfo = nullptr;
        accelerationStructurePointLightWrite.pBufferInfo = nullptr;
        accelerationStructurePointLightWrite.pTexelBufferView = nullptr;


        std::vector<VkDescriptorImageInfo> descriptorImageInfos;
        for (size_t i = 0; i < scene->m_textureCount; i++)
        {
            VkDescriptorImageInfo descriptorImageInfo;
            descriptorImageInfo.sampler = scene->m_images[i]->getSampler(); //nullptr
            descriptorImageInfo.imageView = scene->m_images[i]->getImageView();
            descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            descriptorImageInfos.push_back(descriptorImageInfo);
        }


        VkWriteDescriptorSet descriptorSampler;
        descriptorSampler.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSampler.pNext = nullptr;
        descriptorSampler.dstSet = descriptorSets[i];
        descriptorSampler.dstBinding = 1;
        descriptorSampler.dstArrayElement = 0;
        descriptorSampler.descriptorCount = scene->m_textureCount;
        descriptorSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        descriptorSampler.pImageInfo = descriptorImageInfos.data();
        descriptorSampler.pBufferInfo = nullptr;
        descriptorSampler.pTexelBufferView = nullptr;


        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.push_back(descriptorWrite);
        writeDescriptorSets.push_back(descriptorSampler);
        writeDescriptorSets.push_back(materialDescriptorWrite);
        writeDescriptorSets.push_back(decalDescriptorWrite);
        writeDescriptorSets.push_back(accelerationStructureWrite);
        if (!DISABLE_SCENE_AS)writeDescriptorSets.push_back(accelerationStructureSceneWrite);
        writeDescriptorSets.push_back(pointLightDescriptorWrite);
        writeDescriptorSets.push_back(accelerationStructurePointLightWrite);


        vkUpdateDescriptorSets(context->m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
}


void ForwardRenderer::recordDepthPassCommandBuffers()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        VkResult result = vkBeginCommandBuffer(commandBuffersDepthPass[i], &commandBufferBeginInfo);
        ASSERT_VULKAN(result);

        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = depthPass;
        renderPassBeginInfo.framebuffer = depthPassFramebuffer;
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = { context->m_width, context->m_height };
        VkClearValue depthClearValue = { 1.0f, 0 }; // Clear z buffer in pre pass

        std::vector<VkClearValue> clearValues;
        clearValues.push_back(depthClearValue);


        renderPassBeginInfo.clearValueCount = clearValues.size();
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffersDepthPass[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffersDepthPass[i], VK_PIPELINE_BIND_POINT_GRAPHICS, depthPassPipeline);

        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = context->m_width;
        viewport.height = context->m_height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(commandBuffersDepthPass[i], 0, 1, &viewport);
        VkRect2D scissor;

        scissor.offset = { 0, 0 };
        scissor.extent = { context->m_width, context->m_height };
        vkCmdSetScissor(commandBuffersDepthPass[i], 0, 1, &scissor);


        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffersDepthPass[i], 0, 1, &scene->m_vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffersDepthPass[i], scene->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffersDepthPass[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS, depthPassPipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(commandBuffersDepthPass[i], scene->m_indexCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffersDepthPass[i]);

        result = vkEndCommandBuffer(commandBuffersDepthPass[i]);
        ASSERT_VULKAN(result);
    }
}


void ForwardRenderer::recordCommandBuffers() {
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        VkResult result = vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);
        ASSERT_VULKAN(result);

        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = framebuffers[i];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = { context->m_width, context->m_height };
        VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
        //VkClearValue depthClearValue = {1.0f, 0}; // leave empty

        std::vector<VkClearValue> clearValues;
        clearValues.push_back(clearValue);
        //clearValues.push_back(depthClearValue);


        renderPassBeginInfo.clearValueCount = clearValues.size();
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = context->m_width;
        viewport.height = context->m_height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);
        VkRect2D scissor;

        scissor.offset = { 0, 0 };
        scissor.extent = { context->m_width, context->m_height };
        vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);


        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &scene->m_vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], scene->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

        vkCmdDrawIndexed(commandBuffers[i], scene->m_indexCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        result = vkEndCommandBuffer(commandBuffers[i]);
        ASSERT_VULKAN(result);

    }
}


void ForwardRenderer::createSemaphores() {
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VkResult result = vkCreateSemaphore(context->m_device, &semaphoreCreateInfo, nullptr, &context->semaphoreImageAvailable);
    ASSERT_VULKAN(result);
    result = vkCreateSemaphore(context->m_device, &semaphoreCreateInfo, nullptr, &semaphoreRenderingDone);
    ASSERT_VULKAN(result);
    result = vkCreateSemaphore(context->m_device, &semaphoreCreateInfo, nullptr, &semaphoreDepthPrepassDone);
    ASSERT_VULKAN(result);

}


void ForwardRenderer::drawFrame() {


    //VkResult result = vkAcquireNextImageKHR(context->m_device, context->m_swapchain, std::numeric_limits<uint64_t>::max(), semaphoreImageAvailable, VK_NULL_HANDLE, &context->m_imageRenderIndex);
    //ASSERT_VULKAN(result);



    VkSubmitInfo submitInfo;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffersDepthPass[context->m_imageRenderIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphoreDepthPrepassDone;
    VkResult result = vkQueueSubmit(context->m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    ASSERT_VULKAN(result);




    std::vector<VkSemaphore> waitSemaphores;
    waitSemaphores.push_back(context->semaphoreImageAvailable);
    waitSemaphores.push_back(semaphoreDepthPrepassDone);

    //VkSubmitInfo submitInfo;
    VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &(commandBuffers[context->m_imageRenderIndex]);
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphoreRenderingDone;


    result = vkQueueSubmit(context->m_queue, 1, &submitInfo, context->m_fences[context->m_imageRenderIndex]);
    ASSERT_VULKAN(result);

    VkPresentInfoKHR presentInfo;
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphoreRenderingDone;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &context->m_swapchain;
    presentInfo.pImageIndices = &context->m_imageRenderIndex;
    presentInfo.pResults = nullptr;


    result = vkQueuePresentKHR(context->m_queue, &presentInfo);
    ASSERT_VULKAN(result);
}

void ForwardRenderer::shaderHotReload() {
    vkDeviceWaitIdle(context->m_device);
    vkDestroyShaderModule(context->m_device, shaderModuleVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleFrag, nullptr);
    vkDestroyPipelineLayout(context->m_device, depthPassPipelineLayout, nullptr);
    vkDestroyPipelineLayout(context->m_device, pipelineLayout, nullptr);
    compileShader();
    vkDestroyPipeline(context->m_device, pipeline, nullptr);
    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffers);
    delete[] commandBuffers;
    createPipeline();
    createDepthPassPipeline();
    createCommandBuffers();
    createDepthPassCommandBuffers();
    recordCommandBuffers();
    recordDepthPassCommandBuffers();
}

void ForwardRenderer::destroyForwardPass()
{
    depthImage.destroy();
    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffers);
    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffersDepthPass);
    delete[] commandBuffers;
    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        vkDestroyFramebuffer(context->m_device, framebuffers[i], nullptr);
    }
    vkDestroyFramebuffer(context->m_device, depthPassFramebuffer, nullptr);
    delete[] framebuffers;
    vkDestroyRenderPass(context->m_device, renderPass, nullptr);
    vkDestroyRenderPass(context->m_device, depthPass, nullptr);
}
void ForwardRenderer::restoreForwardPass()
{
    createRenderPass();
    createDepthPass();
    createDepthImage();
    createFramebuffers();
    createFramebuffersDepthPass();
    createCommandBuffers();
    createDepthPassCommandBuffers();
    recordCommandBuffers();
    recordDepthPassCommandBuffers();
}

void ForwardRenderer::destroy()
{
    depthImage.destroy();
    vkDestroyDescriptorSetLayout(context->m_device, descriptorSetLayout, nullptr);

    vkDestroySemaphore(context->m_device, context->semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(context->m_device, semaphoreDepthPrepassDone, nullptr);
    vkDestroySemaphore(context->m_device, semaphoreRenderingDone, nullptr);

    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffers);
    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffersDepthPass);
    delete[] commandBuffers;

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        vkDestroyFramebuffer(context->m_device, framebuffers[i], nullptr);
    }

    vkDestroyFramebuffer(context->m_device, depthPassFramebuffer, nullptr);
    delete[] framebuffers;

    vkDestroyPipeline(context->m_device, pipeline, nullptr);
    vkDestroyPipeline(context->m_device, depthPassPipeline, nullptr);

    vkDestroyRenderPass(context->m_device, renderPass, nullptr);
    vkDestroyRenderPass(context->m_device, depthPass, nullptr);

    vkDestroyPipelineLayout(context->m_device, pipelineLayout, nullptr);
    vkDestroyPipelineLayout(context->m_device, depthPassPipelineLayout, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleFrag, nullptr);

    vkDestroyShaderModule(context->m_device, shaderModuleDepthPassVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleDepthPassFrag, nullptr);
}


