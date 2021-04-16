#pragma once

#include "VulkanUtils.h"
#include "GBuffer.h"
#include "VulkanContext.h"
#include "Scene.h"
#include <iostream>
#include "Vertex.h"
#include "VisibilityBuffer.h"
#include "Cursor.h"
#include "VbRenderer.h"



VbRenderer::VbRenderer(VulkanContext* context, Scene* scene) :
    context(context), scene(scene) {}
VbRenderer::~VbRenderer() {}



void VbRenderer::createRenderPassVb()
{

    std::vector<VkAttachmentDescription> attachments(3);
    VkAttachmentDescription visibilityBufferAttachement = visibilityBuffer.getAttachmentDescriptor(context->m_physicalDevice);
    VkAttachmentDescription depthAttachment = DepthImage::getDepthAttachmentDeferred(context->m_physicalDevice);

    // Swap chain color
    VkAttachmentDescription attachmentDescriptionSwapChainColor;
    attachmentDescriptionSwapChainColor.flags = 0;
    attachmentDescriptionSwapChainColor.format = context->m_colorFormat;
    attachmentDescriptionSwapChainColor.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescriptionSwapChainColor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescriptionSwapChainColor.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescriptionSwapChainColor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescriptionSwapChainColor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescriptionSwapChainColor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescriptionSwapChainColor.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;



    // Swap Chain color attachement
    attachments[0] = attachmentDescriptionSwapChainColor;
    // Input attachements gbuffer
    attachments[1] = visibilityBufferAttachement; // triangle id + draw call id
    // Depth attachement
    attachments[2] = depthAttachment; // depth

    // Currently two subpasses, later one more for decal rendering
    // First subpass: Fill G buffer ------------------------------------------------
    VkAttachmentReference colorAttachmentReferenceGeometryPass;

    // triangle id
    colorAttachmentReferenceGeometryPass.attachment = 1;
    colorAttachmentReferenceGeometryPass.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Depth
    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 2;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    std::vector<VkSubpassDescription> subpassDescriptions(2);
    subpassDescriptions[0].flags = 0;
    subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[0].inputAttachmentCount = 0;
    subpassDescriptions[0].pInputAttachments = nullptr;
    subpassDescriptions[0].colorAttachmentCount = 1;
    subpassDescriptions[0].pColorAttachments = &colorAttachmentReferenceGeometryPass;
    subpassDescriptions[0].pResolveAttachments = nullptr;
    subpassDescriptions[0].pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescriptions[0].preserveAttachmentCount = 0;
    subpassDescriptions[0].pPreserveAttachments = nullptr;


    // second subpass: shading pass ------------------------------------------------
    VkAttachmentReference inputReference;
    inputReference = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };


    VkAttachmentReference colorAttachmentReferenceShadingPass = {};
    colorAttachmentReferenceShadingPass.attachment = 0;
    colorAttachmentReferenceShadingPass.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpassDescriptions[1].flags = 0;
    subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[1].inputAttachmentCount = 1;
    subpassDescriptions[1].pInputAttachments = &inputReference;
    subpassDescriptions[1].colorAttachmentCount = 1;
    subpassDescriptions[1].pColorAttachments = &colorAttachmentReferenceShadingPass;
    subpassDescriptions[1].pResolveAttachments = nullptr;
    subpassDescriptions[1].pDepthStencilAttachment = nullptr;
    subpassDescriptions[1].preserveAttachmentCount = 0;
    subpassDescriptions[1].pPreserveAttachments = nullptr;


    // Subpass dependencies

    std::vector<VkSubpassDependency> dependencies(3); //civ
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //VK_ACCESS_MEMORY_READ_BIT -> VK_ACCESS_SHADER_READ_BIT
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = 1;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


    dependencies[2].srcSubpass = 0;
    dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


    // dependencies TODO civ

    VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = subpassDescriptions.size();
    renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
    renderPassCreateInfo.dependencyCount = dependencies.size();
    renderPassCreateInfo.pDependencies = dependencies.data();

    VkResult result = vkCreateRenderPass(context->m_device, &renderPassCreateInfo, nullptr, &renderPassVb);
    ASSERT_VULKAN(result);
}


void VbRenderer::createDescriptorSetLayoutVbGeometry() {
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



    std::vector<VkDescriptorSetLayoutBinding> descriptorSets;
    descriptorSets.push_back(descriptorSetLayoutBinding);
    descriptorSets.push_back(samplerDescriptorSetLayoutBinding);
    descriptorSets.push_back(materialDescriptorSetLayoutBinding);


    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = descriptorSets.size();
    descriptorSetLayoutCreateInfo.pBindings = descriptorSets.data();


    VkResult result = vkCreateDescriptorSetLayout(context->m_device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayoutVbGeometry);
    ASSERT_VULKAN(result);

}


void VbRenderer::createDescriptorSetLayoutVbShading() {
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding;
    samplerDescriptorSetLayoutBinding.binding = 1;
    samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    samplerDescriptorSetLayoutBinding.descriptorCount = scene->m_textureCount;
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


    VkDescriptorSetLayoutBinding triangleIdInputAttachementDescriptorSetLayoutBinding;
    triangleIdInputAttachementDescriptorSetLayoutBinding.binding = 6;
    triangleIdInputAttachementDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    triangleIdInputAttachementDescriptorSetLayoutBinding.descriptorCount = 1;
    triangleIdInputAttachementDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    triangleIdInputAttachementDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding pointLightDescriptorSetLayoutBinding;
    pointLightDescriptorSetLayoutBinding.binding = 7;
    pointLightDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pointLightDescriptorSetLayoutBinding.descriptorCount = 1;
    pointLightDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pointLightDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding accelerationStructurePointLightDescriptorSetLayoutBinding;
    accelerationStructurePointLightDescriptorSetLayoutBinding.binding = 8;
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
    descriptorSets.push_back(triangleIdInputAttachementDescriptorSetLayoutBinding);
    descriptorSets.push_back(pointLightDescriptorSetLayoutBinding);
    descriptorSets.push_back(accelerationStructurePointLightDescriptorSetLayoutBinding);


    for (size_t i = 0; i < VERTEX_ATTRIBUTE_COUNT; i++)
    {
        VkDescriptorSetLayoutBinding vertexAttributeDescriptorSetLayoutBinding;
        vertexAttributeDescriptorSetLayoutBinding.binding = 8 + 1 + i;
        vertexAttributeDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        vertexAttributeDescriptorSetLayoutBinding.descriptorCount = 1;
        vertexAttributeDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        vertexAttributeDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

        descriptorSets.push_back(vertexAttributeDescriptorSetLayoutBinding);
    }

    VkDescriptorSetLayoutBinding decalCursorDescriptorSetLayoutBinding;
    decalCursorDescriptorSetLayoutBinding.binding = VERTEX_ATTRIBUTE_COUNT + 8 + 1;
    decalCursorDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    decalCursorDescriptorSetLayoutBinding.descriptorCount = 1;
    decalCursorDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    decalCursorDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    descriptorSets.push_back(decalCursorDescriptorSetLayoutBinding);


    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = descriptorSets.size();
    descriptorSetLayoutCreateInfo.pBindings = descriptorSets.data();

    VkResult result = vkCreateDescriptorSetLayout(context->m_device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayoutVbShading);
    ASSERT_VULKAN(result);

}



void VbRenderer::compileShader() {

    std::string cmdShaderCompile = "C:\\VulkanSDK\\1.2.141.2\\Bin\\glslangValidator.exe";


    std::string cmdVbGeometryVertexShaderCompile = cmdShaderCompile
        + " -V -o shader/shader_vb_geom_pass_vert.spv shader/shader_vb_geom_pass.glsl.vert";
    std::string cmdVbGeometryFragmentShaderCompile = cmdShaderCompile;
    cmdVbGeometryFragmentShaderCompile
        .append(" -DTEXTURES=").append(std::to_string(scene->m_textureCount))
        .append(" -DMATERIALS=").append(std::to_string(scene->m_materialCount))
        .append(" -DDECALS=").append(std::to_string(scene->m_decalCount + 1))
        .append(" -V -o shader/shader_vb_geom_pass_frag.spv shader/shader_vb_geom_pass.glsl.frag");

    std::string cmdVbShadingVertexShaderCompile = cmdShaderCompile
        + " -V -o shader/shader_vb_shading_pass_vert.spv shader/shader_vb_shading_pass.glsl.vert";

    std::string cmdVbShadingFragmentShaderCompile = cmdShaderCompile;
    cmdVbShadingFragmentShaderCompile
        .append(" -DTEXTURES=").append(std::to_string(scene->m_textureCount))
        .append(" -DMATERIALS=").append(std::to_string(scene->m_materialCount))
        .append(" -DDECALS=").append(std::to_string(scene->m_decalCount + 1))
        .append(" -DPOINT_LIGHTS=").append(std::to_string(scene->m_pointLightCount))
        .append(createShaderFeatureFlagsComlileArguments(context->shaderFeatureFlags))
        .append(" -V -o shader/shader_vb_shading_pass_frag.spv shader/shader_vb_shading_pass.glsl.frag");

    std::cout << cmdVbGeometryVertexShaderCompile << std::endl;
    std::cout << cmdVbGeometryFragmentShaderCompile << std::endl;
    std::cout << cmdVbShadingVertexShaderCompile << std::endl;
    std::cout << cmdVbShadingFragmentShaderCompile << std::endl;

    system(cmdVbGeometryVertexShaderCompile.c_str());
    system(cmdVbGeometryFragmentShaderCompile.c_str());

    system(cmdVbShadingVertexShaderCompile.c_str());
    system(cmdVbShadingFragmentShaderCompile.c_str());

    auto shaderCodeVbGeometryVert = readFile("shader/shader_vb_geom_pass_vert.spv");
    auto shaderCodeVbGeometryFrag = readFile("shader/shader_vb_geom_pass_frag.spv");

    auto shaderCodeVbShadingVert = readFile("shader/shader_vb_shading_pass_vert.spv");
    auto shaderCodeVbShadingFrag = readFile("shader/shader_vb_shading_pass_frag.spv");

    createShaderModule(context->m_device, shaderCodeVbGeometryVert, &shaderModuleVbGeometryVert);
    createShaderModule(context->m_device, shaderCodeVbGeometryFrag, &shaderModuleVbGeometryFrag);

    createShaderModule(context->m_device, shaderCodeVbShadingVert, &shaderModuleVbShadingVert);
    createShaderModule(context->m_device, shaderCodeVbShadingFrag, &shaderModuleVbShadingFrag);
}


void VbRenderer::createPipelineVbGeometry()
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    shaderStageCreateInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoVert.pNext = nullptr;
    shaderStageCreateInfoVert.flags = 0;
    shaderStageCreateInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfoVert.module = shaderModuleVbGeometryVert;
    shaderStageCreateInfoVert.pName = "main";
    shaderStageCreateInfoVert.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    shaderStageCreateInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoFrag.pNext = nullptr;
    shaderStageCreateInfoFrag.flags = 0;
    shaderStageCreateInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfoFrag.module = shaderModuleVbGeometryFrag;
    shaderStageCreateInfoFrag.pName = "main";
    shaderStageCreateInfoFrag.pSpecializationInfo = nullptr;


    VkPipelineShaderStageCreateInfo shaderStages[] = {
        shaderStageCreateInfoVert,
        shaderStageCreateInfoFrag
    };



    VkVertexInputBindingDescription vertexBindingDescription;
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(glm::vec3);
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexAttributeDescription;
    vertexAttributeDescription.location = 0;
    vertexAttributeDescription.binding = 0;
    vertexAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescription.offset = 0;

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo;
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.pNext = nullptr;
    vertexInputCreateInfo.flags = 0;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 1;
    vertexInputCreateInfo.pVertexAttributeDescriptions = &vertexAttributeDescription;




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


    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.pNext = nullptr;
    depthStencilStateCreateInfo.flags = 0;
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.front = {};
    depthStencilStateCreateInfo.back = {};
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;


    VkPipelineColorBlendAttachmentState colorBlendAttachment;

    colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_FALSE;
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
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutVbGeometry; // TODO fix
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(context->m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayoutVbGeometry);
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
    pipelineCreateInfo.layout = pipelineLayoutVbGeometry;
    pipelineCreateInfo.renderPass = renderPassVb;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(context->m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineVbGeometry);
    ASSERT_VULKAN(result);
}

void VbRenderer::createPipelineVbShading()
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    shaderStageCreateInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoVert.pNext = nullptr;
    shaderStageCreateInfoVert.flags = 0;
    shaderStageCreateInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfoVert.module = shaderModuleVbShadingVert; //TODO fix
    shaderStageCreateInfoVert.pName = "main";
    shaderStageCreateInfoVert.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    shaderStageCreateInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoFrag.pNext = nullptr;
    shaderStageCreateInfoFrag.flags = 0;
    shaderStageCreateInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfoFrag.module = shaderModuleVbShadingFrag; //TODO fix
    shaderStageCreateInfoFrag.pName = "main";
    shaderStageCreateInfoFrag.pSpecializationInfo = nullptr;


    VkPipelineShaderStageCreateInfo shaderStages[] = {
        shaderStageCreateInfoVert,
        shaderStageCreateInfoFrag
    };


    auto vertexBindingDescription = Vertex::getBindingDescription();
    auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

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
    rasterizationCreateInfo.cullMode = VK_CULL_MODE_NONE;
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


    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.pNext = nullptr;
    depthStencilStateCreateInfo.flags = 0;
    depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;//VK_TRUE
    depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.front = {};
    depthStencilStateCreateInfo.back = {};
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;


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
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutVbShading;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(context->m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayoutVbShading);
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
    pipelineCreateInfo.layout = pipelineLayoutVbShading;
    pipelineCreateInfo.renderPass = renderPassVb;
    pipelineCreateInfo.subpass = 1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(context->m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineVbShading);
    ASSERT_VULKAN(result);
}

void VbRenderer::createFramebuffersVb() {

    framebufferVb = new VkFramebuffer[context->m_amountOfImagesInSwapchain];

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {

        VkImageView visibilityBufferAttachmentView = visibilityBuffer.getImageView();
        VkImageView depthBufferAttachmentView = depthImage.getImageView();
        std::vector<VkImageView> attachmentViews;
        attachmentViews.push_back(context->m_imageViews[i]);// Swap chain color
        attachmentViews.push_back(visibilityBufferAttachmentView); // triangle id
        attachmentViews.push_back(depthBufferAttachmentView); // depth


        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPassVb;
        framebufferCreateInfo.attachmentCount = attachmentViews.size();
        framebufferCreateInfo.pAttachments = attachmentViews.data();
        framebufferCreateInfo.width = context->m_width;
        framebufferCreateInfo.height = context->m_height;
        framebufferCreateInfo.layers = 1;


        VkResult result = vkCreateFramebuffer(context->m_device, &framebufferCreateInfo, nullptr, &(framebufferVb[i]));
        ASSERT_VULKAN(result);
    }
}

void VbRenderer::createVisibilityBuffer()
{
    visibilityBuffer.create(context->m_device, context->m_physicalDevice, context->m_commandPool, context->m_queue, context->m_width, context->m_height);
}



void VbRenderer::createDescriptorSetsVbGeometryPass()
{
    std::vector<VkDescriptorSetLayout> layouts(context->m_amountOfImagesInSwapchain, descriptorSetLayoutVbGeometry);
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = context->m_descriptorPool; // Maybe change later
    descriptorSetAllocateInfo.descriptorSetCount = context->m_amountOfImagesInSwapchain;
    descriptorSetAllocateInfo.pSetLayouts = layouts.data();

    descriptorSetsVbGeometry = new VkDescriptorSet[context->m_amountOfImagesInSwapchain];

    VkResult result = vkAllocateDescriptorSets(context->m_device, &descriptorSetAllocateInfo, descriptorSetsVbGeometry);
    ASSERT_VULKAN(result);

    updateDescriptorSetsVbGeometryPass();

}

void VbRenderer::updateDescriptorSetsVbGeometryPass()
{
    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        VkDescriptorBufferInfo descriptorBufferInfo[3];
        descriptorBufferInfo[0].buffer = context->m_uniformBuffers[0];
        descriptorBufferInfo[0].offset = getUniformBufferOffset(sizeof(context->m_perFrameConstants), i, context->m_deviceProperties);
        descriptorBufferInfo[0].range = sizeof(context->m_perFrameConstants);

        descriptorBufferInfo[1].buffer = context->m_uniformBuffers[1];
        descriptorBufferInfo[1].offset = 0;
        descriptorBufferInfo[1].range = VK_WHOLE_SIZE;

        descriptorBufferInfo[2].buffer = context->m_uniformBuffers[2];
        descriptorBufferInfo[2].offset = 0;
        descriptorBufferInfo[2].range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet descriptorWrite;
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.pNext = nullptr;
        descriptorWrite.dstSet = descriptorSetsVbGeometry[i];
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
        materialDescriptorWrite.dstSet = descriptorSetsVbGeometry[i];
        materialDescriptorWrite.dstBinding = 2;
        materialDescriptorWrite.dstArrayElement = 0;
        materialDescriptorWrite.descriptorCount = 1;
        materialDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        materialDescriptorWrite.pImageInfo = nullptr;
        materialDescriptorWrite.pBufferInfo = &(descriptorBufferInfo[1]);
        materialDescriptorWrite.pTexelBufferView = nullptr;

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
        descriptorSampler.dstSet = descriptorSetsVbGeometry[i];
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


        vkUpdateDescriptorSets(context->m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
}



void VbRenderer::createDescriptorSetsVbShadingPass()
{
    std::vector<VkDescriptorSetLayout> layouts(context->m_amountOfImagesInSwapchain, descriptorSetLayoutVbShading);
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = context->m_descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = context->m_amountOfImagesInSwapchain;
    descriptorSetAllocateInfo.pSetLayouts = layouts.data();

    descriptorSetsVbShading = new VkDescriptorSet[context->m_amountOfImagesInSwapchain];

    VkResult result = vkAllocateDescriptorSets(context->m_device, &descriptorSetAllocateInfo, descriptorSetsVbShading);
    ASSERT_VULKAN(result);

    updateDescriptorSetsVbShadingPass();
}

void VbRenderer::updateDescriptorSetsVbShadingPass()
{
    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        VkDescriptorBufferInfo descriptorBufferInfo[5];
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

        descriptorBufferInfo[4].buffer = context->m_uniformBuffers[4];
        descriptorBufferInfo[4].offset = getUniformBufferOffset(sizeof(Cursor), i, context->m_deviceProperties);
        descriptorBufferInfo[4].range = sizeof(Cursor);

        VkWriteDescriptorSet descriptorWrite;
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.pNext = nullptr;
        descriptorWrite.dstSet = descriptorSetsVbShading[i];
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
        materialDescriptorWrite.dstSet = descriptorSetsVbShading[i];
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
        decalDescriptorWrite.dstSet = descriptorSetsVbShading[i];
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
        pointLightDescriptorWrite.dstSet = descriptorSetsVbShading[i];
        pointLightDescriptorWrite.dstBinding = 7;
        pointLightDescriptorWrite.dstArrayElement = 0;
        pointLightDescriptorWrite.descriptorCount = 1;
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
        accelerationStructureWrite.dstSet = descriptorSetsVbShading[i];
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
        accelerationStructureSceneWrite.dstSet = descriptorSetsVbShading[i];
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
        accelerationStructurePointLightWrite.dstSet = descriptorSetsVbShading[i];
        accelerationStructurePointLightWrite.dstBinding = 8;
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
        descriptorSampler.dstSet = descriptorSetsVbShading[i];
        descriptorSampler.dstBinding = 1;
        descriptorSampler.dstArrayElement = 0;
        descriptorSampler.descriptorCount = scene->m_textureCount;
        descriptorSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        descriptorSampler.pImageInfo = descriptorImageInfos.data();
        descriptorSampler.pBufferInfo = nullptr;
        descriptorSampler.pTexelBufferView = nullptr;


        VkDescriptorImageInfo inputAttachementsDescriptorImageInfo;
        VkImageView gBufferView = visibilityBuffer.getImageView();

        inputAttachementsDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputAttachementsDescriptorImageInfo.imageView = gBufferView;
        inputAttachementsDescriptorImageInfo.sampler = VK_NULL_HANDLE;


        VkWriteDescriptorSet inputAttachementsWrite;
        inputAttachementsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inputAttachementsWrite.pNext = nullptr;
        inputAttachementsWrite.dstSet = descriptorSetsVbShading[i];
        inputAttachementsWrite.dstBinding = 6;
        inputAttachementsWrite.dstArrayElement = 0;
        inputAttachementsWrite.descriptorCount = 1;
        inputAttachementsWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputAttachementsWrite.pImageInfo = &inputAttachementsDescriptorImageInfo;
        inputAttachementsWrite.pBufferInfo = nullptr;
        inputAttachementsWrite.pTexelBufferView = nullptr;



        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.push_back(descriptorWrite);
        writeDescriptorSets.push_back(descriptorSampler);
        writeDescriptorSets.push_back(materialDescriptorWrite);
        writeDescriptorSets.push_back(decalDescriptorWrite);
        writeDescriptorSets.push_back(accelerationStructureWrite);
        if (!DISABLE_SCENE_AS)writeDescriptorSets.push_back(accelerationStructureSceneWrite);
        writeDescriptorSets.push_back(inputAttachementsWrite);
        writeDescriptorSets.push_back(pointLightDescriptorWrite);
        writeDescriptorSets.push_back(accelerationStructurePointLightWrite);

        for (size_t j = 0; j < VERTEX_ATTRIBUTE_COUNT; j++)
        {
            VkWriteDescriptorSet vertexAttributeBuffersWrite;
            vertexAttributeBuffersWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            vertexAttributeBuffersWrite.pNext = nullptr;
            vertexAttributeBuffersWrite.dstSet = descriptorSetsVbShading[i];
            vertexAttributeBuffersWrite.dstBinding = 8 + 1 + j;
            vertexAttributeBuffersWrite.dstArrayElement = 0;
            vertexAttributeBuffersWrite.descriptorCount = 1;
            vertexAttributeBuffersWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            vertexAttributeBuffersWrite.pImageInfo = nullptr;
            vertexAttributeBuffersWrite.pBufferInfo = nullptr;
            vertexAttributeBuffersWrite.pTexelBufferView = &(scene->m_vertexAttributeBuffers.views[j]);
            writeDescriptorSets.push_back(vertexAttributeBuffersWrite);
        }

        VkWriteDescriptorSet decalCursorDescriptorWrite;
        decalCursorDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        decalCursorDescriptorWrite.pNext = nullptr;
        decalCursorDescriptorWrite.dstSet = descriptorSetsVbShading[i];
        decalCursorDescriptorWrite.dstBinding = VERTEX_ATTRIBUTE_COUNT + 8 + 1;
        decalCursorDescriptorWrite.dstArrayElement = 0;
        decalCursorDescriptorWrite.descriptorCount = 1;
        decalCursorDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        decalCursorDescriptorWrite.pImageInfo = nullptr;
        decalCursorDescriptorWrite.pBufferInfo = &(descriptorBufferInfo[4]);
        decalCursorDescriptorWrite.pTexelBufferView = nullptr;

        writeDescriptorSets.push_back(decalCursorDescriptorWrite);

        vkUpdateDescriptorSets(context->m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
}


void VbRenderer::recordCommandBuffersVb()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    std::vector<VkClearValue> clearValues(3);
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // swap chain color
    clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // triangle id
    clearValues[2].depthStencil = { 1.0f, 0 }; // depth & stencil

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        VkResult result = vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);
        ASSERT_VULKAN(result);

        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = renderPassVb;
        renderPassBeginInfo.framebuffer = framebufferVb[i];
        renderPassBeginInfo.renderArea.offset = { 0, 0 };
        renderPassBeginInfo.renderArea.extent = { context->m_width, context->m_height };
        renderPassBeginInfo.clearValueCount = clearValues.size();
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
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

        // First Subpass: Render Scene to G-Buffer

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineVbGeometry);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &scene->m_vertexAttributeBuffers.positionBuffer, offsets);
        //vkCmdBindIndexBuffer(commandBuffers[i], scene->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutVbGeometry, 0, 1, &(descriptorSetsVbGeometry[i]), 0, nullptr);
        vkCmdDraw(commandBuffers[i], scene->m_vertexAttributeLists.pos.size(), 1, 0, 0);


        // Second Subpass: Shading Pass
        vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineVbShading);
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutVbShading, 0, 1, &(descriptorSetsVbShading[i]), 0, nullptr);
        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0); // Positions and UVs generated in vertex shader. (see also https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/)

        vkCmdEndRenderPass(commandBuffers[i]);

        result = vkEndCommandBuffer(commandBuffers[i]);
        ASSERT_VULKAN(result);

    }
}

void VbRenderer::shaderHotReload() {
    vkDeviceWaitIdle(context->m_device);
    vkDestroyShaderModule(context->m_device, shaderModuleVbGeometryVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleVbGeometryFrag, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleVbShadingVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleVbShadingFrag, nullptr);
    vkDestroyPipelineLayout(context->m_device, pipelineLayoutVbGeometry, nullptr);
    vkDestroyPipelineLayout(context->m_device, pipelineLayoutVbShading, nullptr);
    vkDestroyPipeline(context->m_device, pipelineVbGeometry, nullptr);
    vkDestroyPipeline(context->m_device, pipelineVbShading, nullptr);
    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffers);
    delete[] commandBuffers;
    compileShader();
    createPipelineVbGeometry();
    createPipelineVbShading();
    createCommandBuffers();
    recordCommandBuffersVb();
}


void VbRenderer::drawFrameVb()
{


    //VkResult result = vkAcquireNextImageKHR(context->m_device, context->m_swapchain, std::numeric_limits<uint64_t>::max(), semaphoreImageAvailable, VK_NULL_HANDLE, &context->m_imageRenderIndex);
    //ASSERT_VULKAN(result);




    std::vector<VkSemaphore> waitSemaphores;
    waitSemaphores.push_back(context->semaphoreImageAvailable);

    //VkSubmitInfo submitInfo;
    VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo;
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


    VkResult result = vkQueueSubmit(context->m_queue, 1, &submitInfo, context->m_fences[context->m_imageRenderIndex]);
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



void VbRenderer::createCommandBuffers() {
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


void VbRenderer::createSemaphores() {
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VkResult result = vkCreateSemaphore(context->m_device, &semaphoreCreateInfo, nullptr, &context->semaphoreImageAvailable);
    ASSERT_VULKAN(result);
    result = vkCreateSemaphore(context->m_device, &semaphoreCreateInfo, nullptr, &semaphoreRenderingDone);
    ASSERT_VULKAN(result);

}



void VbRenderer::destroyVbPass()
{
    visibilityBuffer.destroy();
    depthImage.destroy();
    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffers);
    delete[] commandBuffers;
    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        vkDestroyFramebuffer(context->m_device, framebufferVb[i], nullptr);
    }
    delete[] framebufferVb;
    vkDestroyRenderPass(context->m_device, renderPassVb, nullptr);
}
void VbRenderer::restoreVbPass()
{
    createRenderPassVb();
    createVisibilityBuffer();
    createDepthImage();
    createFramebuffersVb();
    createCommandBuffers();

    updateDescriptorSetsVbShadingPass();
    updateDescriptorSetsVbGeometryPass();

    recordCommandBuffersVb();
}



void VbRenderer::destroy()
{





    visibilityBuffer.destroy();
    depthImage.destroy();
    vkDestroyDescriptorSetLayout(context->m_device, descriptorSetLayoutVbGeometry, nullptr);
    vkDestroyDescriptorSetLayout(context->m_device, descriptorSetLayoutVbShading, nullptr);

    vkDestroySemaphore(context->m_device, context->semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(context->m_device, semaphoreRenderingDone, nullptr);

    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffers);
    delete[] commandBuffers;

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        vkDestroyFramebuffer(context->m_device, framebufferVb[i], nullptr);
    }
    delete[] framebufferVb;

    vkDestroyPipeline(context->m_device, pipelineVbGeometry, nullptr);
    vkDestroyPipeline(context->m_device, pipelineVbShading, nullptr);

    vkDestroyRenderPass(context->m_device, renderPassVb, nullptr);

    vkDestroyPipelineLayout(context->m_device, pipelineLayoutVbGeometry, nullptr);
    vkDestroyPipelineLayout(context->m_device, pipelineLayoutVbShading, nullptr);

    vkDestroyShaderModule(context->m_device, shaderModuleVbGeometryVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleVbGeometryFrag, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleVbShadingVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleVbShadingFrag, nullptr);
}




void VbRenderer::createDepthImage()
{
    depthImage.create(context->m_device, context->m_physicalDevice, context->m_commandPool, context->m_queue, context->m_width, context->m_height);
}