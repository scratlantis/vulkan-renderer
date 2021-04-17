#pragma once

#include "VulkanUtils.h"
#include "GBuffer.h"
#include "VulkanContext.h"
#include "DeferredRenderer.h"
#include "Scene.h"
#include <iostream>
#include "Vertex.h"

using namespace VulkanUtils;


DeferredRenderer::DeferredRenderer(VulkanContext* context, Scene* scene, uint32_t flags) :
    context(context), scene(scene), flags(flags) {}
DeferredRenderer::~DeferredRenderer()
{

}

void DeferredRenderer::createRenderPassDeferred()
{

    std::vector<VkAttachmentDescription> attachments(5);
    std::vector<VkAttachmentDescription> gBufferAttachements = gBuffer.getAttachmentDescriptors(context->m_physicalDevice);
    VkAttachmentDescription depthAttachment = gBufferAttachements[3];

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
    attachments[1] = gBufferAttachements[0]; // albedo
    attachments[2] = gBufferAttachements[1]; // normals
    attachments[3] = gBufferAttachements[2]; // Rougness + Metalicity
    // Depth attachement
    attachments[4] = depthAttachment; // depth


    uint32_t subpassIndex = 0;
    std::vector<VkSubpassDescription> subpassDescriptions;
    std::vector<VkSubpassDependency> dependencies;

    // Currently two subpasses, later one more for decal rendering
    // First subpass: Fill G buffer ------------------------------------------------


    VkSubpassDependency dependencieInitial;
    dependencieInitial.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencieInitial.dstSubpass = 0;
    dependencieInitial.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencieInitial.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencieInitial.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencieInitial.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencieInitial.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(dependencieInitial);

    std::vector<VkAttachmentReference> colorAttachmentReferenceGeometryPass(3);

    // Albedo
    colorAttachmentReferenceGeometryPass[0].attachment = 1;
    colorAttachmentReferenceGeometryPass[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Normals
    colorAttachmentReferenceGeometryPass[1].attachment = 2;
    colorAttachmentReferenceGeometryPass[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Rougness + Metalicity
    colorAttachmentReferenceGeometryPass[2].attachment = 3;
    colorAttachmentReferenceGeometryPass[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Depth
    VkAttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 4;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescriptionGeometryPass;
    subpassDescriptionGeometryPass.flags = 0;
    subpassDescriptionGeometryPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptionGeometryPass.inputAttachmentCount = 0;
    subpassDescriptionGeometryPass.pInputAttachments = nullptr;
    subpassDescriptionGeometryPass.colorAttachmentCount = colorAttachmentReferenceGeometryPass.size();
    subpassDescriptionGeometryPass.pColorAttachments = colorAttachmentReferenceGeometryPass.data();
    subpassDescriptionGeometryPass.pResolveAttachments = nullptr;
    subpassDescriptionGeometryPass.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescriptionGeometryPass.preserveAttachmentCount = 0;
    subpassDescriptionGeometryPass.pPreserveAttachments = nullptr;
    subpassDescriptions.push_back(subpassDescriptionGeometryPass);
    subpassIndex++;
    // subpass render deferred decals --------------------------------------------
    VkSubpassDependency dependencieDeferredDecalPass;
    dependencieDeferredDecalPass.srcSubpass = 0;
    dependencieDeferredDecalPass.dstSubpass = 1;
    dependencieDeferredDecalPass.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencieDeferredDecalPass.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencieDeferredDecalPass.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencieDeferredDecalPass.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    dependencieDeferredDecalPass.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(dependencieDeferredDecalPass);

    // Reads
    std::vector<VkAttachmentReference> inputReferenceDeferredDecalPass(1);
    inputReferenceDeferredDecalPass[0] = { 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    //inputReferenceDeferredDecalPass[1] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    //inputReferenceDeferredDecalPass[2] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };


    // Writes
    std::vector<VkAttachmentReference> colorAttachmentReferenceDeferredDecalPass(3);
    // Albedo
    colorAttachmentReferenceDeferredDecalPass[0].attachment = 1;
    colorAttachmentReferenceDeferredDecalPass[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Normals
    colorAttachmentReferenceDeferredDecalPass[1].attachment = 2;
    colorAttachmentReferenceDeferredDecalPass[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Roughness + Metalicity
    colorAttachmentReferenceDeferredDecalPass[2].attachment = 3;
    colorAttachmentReferenceDeferredDecalPass[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Depth
    //depthAttachmentReference.attachment = 4;
    //depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescriptionDeferredDecalPass;
    subpassDescriptionDeferredDecalPass.flags = 0;
    subpassDescriptionDeferredDecalPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptionDeferredDecalPass.inputAttachmentCount = inputReferenceDeferredDecalPass.size();
    subpassDescriptionDeferredDecalPass.pInputAttachments = inputReferenceDeferredDecalPass.data();
    subpassDescriptionDeferredDecalPass.colorAttachmentCount = colorAttachmentReferenceDeferredDecalPass.size();
    subpassDescriptionDeferredDecalPass.pColorAttachments = colorAttachmentReferenceDeferredDecalPass.data();
    subpassDescriptionDeferredDecalPass.pResolveAttachments = nullptr;
    subpassDescriptionDeferredDecalPass.pDepthStencilAttachment = nullptr;
    subpassDescriptionDeferredDecalPass.preserveAttachmentCount = 0;
    subpassDescriptionDeferredDecalPass.pPreserveAttachments = nullptr;
    if (flags & DEFERRED_DECALS)
    {
        subpassDescriptions.push_back(subpassDescriptionDeferredDecalPass);
        subpassIndex++;
    }
    // last subpass: shading pass ------------------------------------------------
    VkSubpassDependency dependencieShadingPass;
    dependencieShadingPass.srcSubpass = subpassIndex - 1;
    dependencieShadingPass.dstSubpass = subpassIndex;
    dependencieShadingPass.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencieShadingPass.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencieShadingPass.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencieShadingPass.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencieShadingPass.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(dependencieShadingPass);

    std::vector<VkAttachmentReference> inputReference(4);
    inputReference[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReference[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReference[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReference[3] = { 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    VkAttachmentReference colorAttachmentReferenceShadingPass = {};
    colorAttachmentReferenceShadingPass.attachment = 0;
    colorAttachmentReferenceShadingPass.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescriptionShadingPass;
    subpassDescriptionShadingPass.flags = 0;
    subpassDescriptionShadingPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptionShadingPass.inputAttachmentCount = inputReference.size();
    subpassDescriptionShadingPass.pInputAttachments = inputReference.data();
    subpassDescriptionShadingPass.colorAttachmentCount = 1;
    subpassDescriptionShadingPass.pColorAttachments = &colorAttachmentReferenceShadingPass;
    subpassDescriptionShadingPass.pResolveAttachments = nullptr;
    subpassDescriptionShadingPass.pDepthStencilAttachment = nullptr;
    subpassDescriptionShadingPass.preserveAttachmentCount = 0;
    subpassDescriptionShadingPass.pPreserveAttachments = nullptr;
    subpassDescriptions.push_back(subpassDescriptionShadingPass);


    VkSubpassDependency dependencieFinal;
    dependencieFinal.srcSubpass = 0;
    dependencieFinal.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencieFinal.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencieFinal.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencieFinal.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencieFinal.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencieFinal.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies.push_back(dependencieFinal);


    VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = attachments.size();
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = subpassDescriptions.size();
    renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
    renderPassCreateInfo.dependencyCount = dependencies.size();
    renderPassCreateInfo.pDependencies = dependencies.data();

    VkResult result = vkCreateRenderPass(context->m_device, &renderPassCreateInfo, nullptr, &renderPassDeferred);
    ASSERT_VULKAN(result);
}


void DeferredRenderer::createDescriptorSetLayoutDeferredGeometry() {
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


    VkResult result = vkCreateDescriptorSetLayout(context->m_device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayoutDeferredGeometry);
    ASSERT_VULKAN(result);

}


void DeferredRenderer::createDescriptorSetLayoutDeferredDecal() {
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
    decalDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    decalDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding depthInputAttachementDescriptorSetLayoutBinding;
    depthInputAttachementDescriptorSetLayoutBinding.binding = 4;
    depthInputAttachementDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthInputAttachementDescriptorSetLayoutBinding.descriptorCount = 1;
    depthInputAttachementDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    depthInputAttachementDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> descriptorSets;
    descriptorSets.push_back(descriptorSetLayoutBinding);
    descriptorSets.push_back(samplerDescriptorSetLayoutBinding);
    descriptorSets.push_back(materialDescriptorSetLayoutBinding);
    descriptorSets.push_back(decalDescriptorSetLayoutBinding);
    descriptorSets.push_back(depthInputAttachementDescriptorSetLayoutBinding);


    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = descriptorSets.size();
    descriptorSetLayoutCreateInfo.pBindings = descriptorSets.data();


    VkResult result = vkCreateDescriptorSetLayout(context->m_device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayoutDeferredDecal);
    ASSERT_VULKAN(result);

}


void DeferredRenderer::createDescriptorSetLayoutDeferredShading() {
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


    VkDescriptorSetLayoutBinding albedoInputAttachementDescriptorSetLayoutBinding;
    albedoInputAttachementDescriptorSetLayoutBinding.binding = 6;
    albedoInputAttachementDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    albedoInputAttachementDescriptorSetLayoutBinding.descriptorCount = 1;
    albedoInputAttachementDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    albedoInputAttachementDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding normalInputAttachementDescriptorSetLayoutBinding;
    normalInputAttachementDescriptorSetLayoutBinding.binding = 7;
    normalInputAttachementDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    normalInputAttachementDescriptorSetLayoutBinding.descriptorCount = 1;
    normalInputAttachementDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    normalInputAttachementDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding roughnessMetalicityInputAttachementDescriptorSetLayoutBinding;
    roughnessMetalicityInputAttachementDescriptorSetLayoutBinding.binding = 8;
    roughnessMetalicityInputAttachementDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    roughnessMetalicityInputAttachementDescriptorSetLayoutBinding.descriptorCount = 1;
    roughnessMetalicityInputAttachementDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    roughnessMetalicityInputAttachementDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding depthInputAttachementDescriptorSetLayoutBinding;
    depthInputAttachementDescriptorSetLayoutBinding.binding = 9;
    depthInputAttachementDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthInputAttachementDescriptorSetLayoutBinding.descriptorCount = 1;
    depthInputAttachementDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    depthInputAttachementDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding pointLightDescriptorSetLayoutBinding;
    pointLightDescriptorSetLayoutBinding.binding = 10;
    pointLightDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pointLightDescriptorSetLayoutBinding.descriptorCount = 1;
    pointLightDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pointLightDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding accelerationStructurePointLightDescriptorSetLayoutBinding;
    accelerationStructurePointLightDescriptorSetLayoutBinding.binding = 11;
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
    descriptorSets.push_back(albedoInputAttachementDescriptorSetLayoutBinding);
    descriptorSets.push_back(normalInputAttachementDescriptorSetLayoutBinding);
    descriptorSets.push_back(roughnessMetalicityInputAttachementDescriptorSetLayoutBinding);
    descriptorSets.push_back(depthInputAttachementDescriptorSetLayoutBinding);
    descriptorSets.push_back(pointLightDescriptorSetLayoutBinding);
    descriptorSets.push_back(accelerationStructurePointLightDescriptorSetLayoutBinding);


    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.bindingCount = descriptorSets.size();
    descriptorSetLayoutCreateInfo.pBindings = descriptorSets.data();

    VkResult result = vkCreateDescriptorSetLayout(context->m_device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayoutDeferredShading);
    ASSERT_VULKAN(result);

}


void DeferredRenderer::compileShader() {

    std::string cmdShaderCompile = VULKAN_PATH;
    cmdShaderCompile.append("Bin\\glslangValidator.exe");

    // Geometry Pass
    std::string cmdDeferredGeometryVertexShaderCompile = cmdShaderCompile
        + " -V -o shader/shader_deferred_geom_pass_vert.spv shader/shader_deferred_geom_pass.glsl.vert";
    std::string cmdDeferredGeometryFragmentShaderCompile = cmdShaderCompile;
    cmdDeferredGeometryFragmentShaderCompile
        .append(" -DTEXTURES=").append(std::to_string(scene->m_textureCount))
        .append(" -DMATERIALS=").append(std::to_string(scene->m_materialCount))
        .append(" -DDECALS=").append(std::to_string(scene->m_decalCount + 1))
        .append(" -V -o shader/shader_deferred_geom_pass_frag.spv shader/shader_deferred_geom_pass.glsl.frag");
    std::cout << cmdDeferredGeometryVertexShaderCompile << std::endl;
    std::cout << cmdDeferredGeometryFragmentShaderCompile << std::endl;
    system(cmdDeferredGeometryVertexShaderCompile.c_str());
    system(cmdDeferredGeometryFragmentShaderCompile.c_str());
    auto shaderCodeDeferredGeometryVert = readFile("shader/shader_deferred_geom_pass_vert.spv");
    auto shaderCodeDeferredGeometryFrag = readFile("shader/shader_deferred_geom_pass_frag.spv");
    createShaderModule(context->m_device, shaderCodeDeferredGeometryVert, &shaderModuleDeferredGeometryVert);
    createShaderModule(context->m_device, shaderCodeDeferredGeometryFrag, &shaderModuleDeferredGeometryFrag);
    // Decal Pass
    if (flags & DEFERRED_DECALS)
    {
        std::string cmdDeferredDecalVertexShaderCompile = cmdShaderCompile;
        cmdDeferredDecalVertexShaderCompile
            .append(" -DDECALS=").append(std::to_string(scene->m_decalCount + 1))
            .append(" -V -o shader/shader_deferred_decal_pass_vert.spv shader/shader_deferred_decal_pass.glsl.vert");
        std::string cmdDeferredDecalFragmentShaderCompile = cmdShaderCompile;
        cmdDeferredDecalFragmentShaderCompile
            .append(" -DTEXTURES=").append(std::to_string(scene->m_textureCount))
            .append(" -DMATERIALS=").append(std::to_string(scene->m_materialCount))
            .append(" -DDECALS=").append(std::to_string(scene->m_decalCount + 1))
            .append(" -V -o shader/shader_deferred_decal_pass_frag.spv shader/shader_deferred_decal_pass.glsl.frag");


        std::cout << cmdDeferredDecalVertexShaderCompile << std::endl;
        std::cout << cmdDeferredDecalFragmentShaderCompile << std::endl;
        system(cmdDeferredDecalVertexShaderCompile.c_str());
        system(cmdDeferredDecalFragmentShaderCompile.c_str());
        auto shaderCodeDeferredDecalVert = readFile("shader/shader_deferred_decal_pass_vert.spv");
        auto shaderCodeDeferredDecalFrag = readFile("shader/shader_deferred_decal_pass_frag.spv");
        createShaderModule(context->m_device, shaderCodeDeferredDecalVert, &shaderModuleDeferredDecalVert);
        createShaderModule(context->m_device, shaderCodeDeferredDecalFrag, &shaderModuleDeferredDecalFrag);

    }
    // Shading Pass
    std::string cmdDeferredShadingVertexShaderCompile = cmdShaderCompile
        + " -V -o shader/shader_deferred_shading_pass_vert.spv shader/shader_deferred_shading_pass.glsl.vert";
    std::string cmdDeferredShadingFragmentShaderCompile = cmdShaderCompile;
    cmdDeferredShadingFragmentShaderCompile
        .append(" -DTEXTURES=").append(std::to_string(scene->m_textureCount))
        .append(" -DMATERIALS=").append(std::to_string(scene->m_materialCount))
        .append(" -DDECALS=").append(std::to_string(scene->m_decalCount + 1))
        .append(" -DDEFERRED_DECALS=").append(std::to_string((bool)(flags & DEFERRED_DECALS)))
        .append(" -DPOINT_LIGHTS=").append(std::to_string(scene->m_pointLightCount))
        .append(createShaderFeatureFlagsComlileArguments(context->shaderFeatureFlags))
        .append(" -V -o shader/shader_deferred_shading_pass_frag.spv shader/shader_deferred_shading_pass.glsl.frag");
    std::cout << cmdDeferredShadingVertexShaderCompile << std::endl;
    std::cout << cmdDeferredShadingFragmentShaderCompile << std::endl;
    system(cmdDeferredShadingVertexShaderCompile.c_str());
    system(cmdDeferredShadingFragmentShaderCompile.c_str());
    auto shaderCodeDeferredShadingVert = readFile("shader/shader_deferred_shading_pass_vert.spv");
    auto shaderCodeDeferredShadingFrag = readFile("shader/shader_deferred_shading_pass_frag.spv");
    createShaderModule(context->m_device, shaderCodeDeferredShadingVert, &shaderModuleDeferredShadingVert);
    createShaderModule(context->m_device, shaderCodeDeferredShadingFrag, &shaderModuleDeferredShadingFrag);
}


void DeferredRenderer::createPipelineDeferredGeometry()
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    shaderStageCreateInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoVert.pNext = nullptr;
    shaderStageCreateInfoVert.flags = 0;
    shaderStageCreateInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfoVert.module = shaderModuleDeferredGeometryVert;
    shaderStageCreateInfoVert.pName = "main";
    shaderStageCreateInfoVert.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    shaderStageCreateInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoFrag.pNext = nullptr;
    shaderStageCreateInfoFrag.flags = 0;
    shaderStageCreateInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfoFrag.module = shaderModuleDeferredGeometryFrag;
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


    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachment(3);
    for (size_t i = 0; i < colorBlendAttachment.size(); i++)
    {
        colorBlendAttachment[i] = {};
        colorBlendAttachment[i].blendEnable = VK_FALSE;
        colorBlendAttachment[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.pNext = nullptr;
    colorBlendCreateInfo.flags = 0;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
    colorBlendCreateInfo.attachmentCount = colorBlendAttachment.size();
    colorBlendCreateInfo.pAttachments = colorBlendAttachment.data();
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
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutDeferredGeometry; // TODO fix
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(context->m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayoutDeferredGeometry);
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
    pipelineCreateInfo.layout = pipelineLayoutDeferredGeometry;
    pipelineCreateInfo.renderPass = renderPassDeferred;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(context->m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineDeferredGeometry);
    ASSERT_VULKAN(result);
}



//Use Fragmentshader interlock?
void DeferredRenderer::createPipelineDeferredDecal()
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    shaderStageCreateInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoVert.pNext = nullptr;
    shaderStageCreateInfoVert.flags = 0;
    shaderStageCreateInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfoVert.module = shaderModuleDeferredDecalVert;
    shaderStageCreateInfoVert.pName = "main";
    shaderStageCreateInfoVert.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    shaderStageCreateInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoFrag.pNext = nullptr;
    shaderStageCreateInfoFrag.flags = 0;
    shaderStageCreateInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfoFrag.module = shaderModuleDeferredDecalFrag;
    shaderStageCreateInfoFrag.pName = "main";
    shaderStageCreateInfoFrag.pSpecializationInfo = nullptr;


    VkPipelineShaderStageCreateInfo shaderStages[] = {
        shaderStageCreateInfoVert,
        shaderStageCreateInfoFrag
    };


    auto vertexBindingDescription = Vertex::getBindingDescription(); //ToDo Maybe define custom vertex for decals
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


    VkPipelineRasterizationDepthClipStateCreateInfoEXT pipelineRasterizationDepthClipStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT };
    pipelineRasterizationDepthClipStateCreateInfo.pNext = nullptr;
    pipelineRasterizationDepthClipStateCreateInfo.flags = 0;
    pipelineRasterizationDepthClipStateCreateInfo.depthClipEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo;
    rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationCreateInfo.pNext = &pipelineRasterizationDepthClipStateCreateInfo;
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
    depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.front = {};
    depthStencilStateCreateInfo.back = {};
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;


    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(3);
    for (size_t i = 0; i < colorBlendAttachments.size(); i++)
    {
        colorBlendAttachments[i].blendEnable = VK_TRUE;
        colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo;
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.pNext = nullptr;
    colorBlendCreateInfo.flags = 0;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_NO_OP;
    colorBlendCreateInfo.attachmentCount = colorBlendAttachments.size();
    colorBlendCreateInfo.pAttachments = colorBlendAttachments.data();
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
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutDeferredDecal;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(context->m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayoutDeferredDecal);
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
    pipelineCreateInfo.layout = pipelineLayoutDeferredDecal;
    pipelineCreateInfo.renderPass = renderPassDeferred;
    pipelineCreateInfo.subpass = 1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(context->m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineDeferredDecal);
    ASSERT_VULKAN(result);
}




void DeferredRenderer::createPipelineDeferredShading()
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfoVert;
    shaderStageCreateInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoVert.pNext = nullptr;
    shaderStageCreateInfoVert.flags = 0;
    shaderStageCreateInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfoVert.module = shaderModuleDeferredShadingVert; //TODO fix
    shaderStageCreateInfoVert.pName = "main";
    shaderStageCreateInfoVert.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfoFrag;
    shaderStageCreateInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfoFrag.pNext = nullptr;
    shaderStageCreateInfoFrag.flags = 0;
    shaderStageCreateInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfoFrag.module = shaderModuleDeferredShadingFrag; //TODO fix
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
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayoutDeferredShading;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult result = vkCreatePipelineLayout(context->m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayoutDeferredShading);
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
    pipelineCreateInfo.layout = pipelineLayoutDeferredShading;
    pipelineCreateInfo.renderPass = renderPassDeferred;
    pipelineCreateInfo.subpass = (flags & DEFERRED_DECALS) ? 2 : 1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(context->m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelineDeferredShading);
    ASSERT_VULKAN(result);
}

void DeferredRenderer::createFramebuffersDeferred() {

    framebufferDeferred = new VkFramebuffer[context->m_amountOfImagesInSwapchain];

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {

        std::vector<VkImageView> gBufferAttachmentViews = gBuffer.getImageViews();
        std::vector<VkImageView> attachmentViews;
        attachmentViews.push_back(context->m_imageViews[i]);// Swap chain color
        attachmentViews.push_back(gBufferAttachmentViews[0]); // albedo
        attachmentViews.push_back(gBufferAttachmentViews[1]); // normals
        attachmentViews.push_back(gBufferAttachmentViews[2]); // Roughness + Metalicity
        attachmentViews.push_back(gBufferAttachmentViews[3]); // depth


        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPassDeferred;
        framebufferCreateInfo.attachmentCount = attachmentViews.size();
        framebufferCreateInfo.pAttachments = attachmentViews.data();
        framebufferCreateInfo.width = context->m_width;
        framebufferCreateInfo.height = context->m_height;
        framebufferCreateInfo.layers = 1;


        VkResult result = vkCreateFramebuffer(context->m_device, &framebufferCreateInfo, nullptr, &(framebufferDeferred[i]));
        ASSERT_VULKAN(result);
    }
}

void DeferredRenderer::createGBuffer()
{
    gBuffer.create(context->m_device, context->m_physicalDevice, context->m_commandPool, context->m_queue, context->m_width, context->m_height);
}



void DeferredRenderer::createDescriptorSetsDeferredGeometryPass()
{
    std::vector<VkDescriptorSetLayout> layouts(context->m_amountOfImagesInSwapchain, descriptorSetLayoutDeferredGeometry);
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = context->m_descriptorPool; // Maybe change later
    descriptorSetAllocateInfo.descriptorSetCount = context->m_amountOfImagesInSwapchain;
    descriptorSetAllocateInfo.pSetLayouts = layouts.data();

    descriptorSetsDeferredGeometry = new VkDescriptorSet[context->m_amountOfImagesInSwapchain];

    VkResult result = vkAllocateDescriptorSets(context->m_device, &descriptorSetAllocateInfo, descriptorSetsDeferredGeometry);
    ASSERT_VULKAN(result);

    updateDescriptorSetsDeferredGeometryPass();

}

void DeferredRenderer::updateDescriptorSetsDeferredGeometryPass()
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
        descriptorWrite.dstSet = descriptorSetsDeferredGeometry[i];
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
        materialDescriptorWrite.dstSet = descriptorSetsDeferredGeometry[i];
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
        descriptorSampler.dstSet = descriptorSetsDeferredGeometry[i];
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







void DeferredRenderer::createDescriptorSetsDeferredDecalPass()
{
    std::vector<VkDescriptorSetLayout> layouts(context->m_amountOfImagesInSwapchain, descriptorSetLayoutDeferredDecal);
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = context->m_descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = context->m_amountOfImagesInSwapchain;
    descriptorSetAllocateInfo.pSetLayouts = layouts.data();

    descriptorSetsDeferredDecal = new VkDescriptorSet[context->m_amountOfImagesInSwapchain];

    VkResult result = vkAllocateDescriptorSets(context->m_device, &descriptorSetAllocateInfo, descriptorSetsDeferredDecal);
    ASSERT_VULKAN(result);

    updateDescriptorSetsDeferredDecalPass();
}

void DeferredRenderer::updateDescriptorSetsDeferredDecalPass()
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
        descriptorWrite.dstSet = descriptorSetsDeferredDecal[i];
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
        materialDescriptorWrite.dstSet = descriptorSetsDeferredDecal[i];
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
        decalDescriptorWrite.dstSet = descriptorSetsDeferredDecal[i];
        decalDescriptorWrite.dstBinding = 3;
        decalDescriptorWrite.dstArrayElement = 0;
        decalDescriptorWrite.descriptorCount = 1;
        decalDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        decalDescriptorWrite.pImageInfo = nullptr;
        decalDescriptorWrite.pBufferInfo = &(descriptorBufferInfo[2]);
        decalDescriptorWrite.pTexelBufferView = nullptr;



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
        descriptorSampler.dstSet = descriptorSetsDeferredDecal[i];
        descriptorSampler.dstBinding = 1;
        descriptorSampler.dstArrayElement = 0;
        descriptorSampler.descriptorCount = scene->m_textureCount;
        descriptorSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        descriptorSampler.pImageInfo = descriptorImageInfos.data();
        descriptorSampler.pBufferInfo = nullptr;
        descriptorSampler.pTexelBufferView = nullptr;


        std::vector<VkDescriptorImageInfo> inputAttachementsDescriptorImageInfos(1);
        std::vector<VkImageView> gBufferViews = gBuffer.getImageViews();

        inputAttachementsDescriptorImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputAttachementsDescriptorImageInfos[0].imageView = gBufferViews[3];
        inputAttachementsDescriptorImageInfos[0].sampler = VK_NULL_HANDLE;

        std::vector<VkWriteDescriptorSet> inputAttachementsWrites(1);
        inputAttachementsWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inputAttachementsWrites[0].pNext = nullptr;
        inputAttachementsWrites[0].dstSet = descriptorSetsDeferredDecal[i];
        inputAttachementsWrites[0].dstBinding = 4;
        inputAttachementsWrites[0].dstArrayElement = 0;
        inputAttachementsWrites[0].descriptorCount = 1;
        inputAttachementsWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputAttachementsWrites[0].pImageInfo = &(inputAttachementsDescriptorImageInfos[0]);
        inputAttachementsWrites[0].pBufferInfo = nullptr;
        inputAttachementsWrites[0].pTexelBufferView = nullptr;



        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.push_back(descriptorWrite);
        writeDescriptorSets.push_back(descriptorSampler);
        writeDescriptorSets.push_back(materialDescriptorWrite);
        writeDescriptorSets.push_back(decalDescriptorWrite);
        writeDescriptorSets.push_back(inputAttachementsWrites[0]);


        vkUpdateDescriptorSets(context->m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
}



void DeferredRenderer::createDescriptorSetsDeferredShadingPass()
{
    std::vector<VkDescriptorSetLayout> layouts(context->m_amountOfImagesInSwapchain, descriptorSetLayoutDeferredShading);
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = nullptr;
    descriptorSetAllocateInfo.descriptorPool = context->m_descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = context->m_amountOfImagesInSwapchain;
    descriptorSetAllocateInfo.pSetLayouts = layouts.data();

    descriptorSetsDeferredShading = new VkDescriptorSet[context->m_amountOfImagesInSwapchain];

    VkResult result = vkAllocateDescriptorSets(context->m_device, &descriptorSetAllocateInfo, descriptorSetsDeferredShading);
    ASSERT_VULKAN(result);

    updateDescriptorSetsDeferredShadingPass();
}

void DeferredRenderer::updateDescriptorSetsDeferredShadingPass()
{
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
        descriptorWrite.dstSet = descriptorSetsDeferredShading[i];
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
        materialDescriptorWrite.dstSet = descriptorSetsDeferredShading[i];
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
        decalDescriptorWrite.dstSet = descriptorSetsDeferredShading[i];
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
        pointLightDescriptorWrite.dstSet = descriptorSetsDeferredShading[i];
        pointLightDescriptorWrite.dstBinding = 10;
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
        accelerationStructureWrite.dstSet = descriptorSetsDeferredShading[i];
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
        accelerationStructureSceneWrite.dstSet = descriptorSetsDeferredShading[i];
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
        accelerationStructurePointLightWrite.dstSet = descriptorSetsDeferredShading[i];
        accelerationStructurePointLightWrite.dstBinding = 11;
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
        descriptorSampler.dstSet = descriptorSetsDeferredShading[i];
        descriptorSampler.dstBinding = 1;
        descriptorSampler.dstArrayElement = 0;
        descriptorSampler.descriptorCount = scene->m_textureCount;
        descriptorSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        descriptorSampler.pImageInfo = descriptorImageInfos.data();
        descriptorSampler.pBufferInfo = nullptr;
        descriptorSampler.pTexelBufferView = nullptr;


        std::vector<VkDescriptorImageInfo> inputAttachementsDescriptorImageInfos(4);
        std::vector<VkImageView> gBufferViews = gBuffer.getImageViews();

        inputAttachementsDescriptorImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputAttachementsDescriptorImageInfos[0].imageView = gBufferViews[0];
        inputAttachementsDescriptorImageInfos[0].sampler = VK_NULL_HANDLE;

        inputAttachementsDescriptorImageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputAttachementsDescriptorImageInfos[1].imageView = gBufferViews[1];
        inputAttachementsDescriptorImageInfos[1].sampler = VK_NULL_HANDLE;

        inputAttachementsDescriptorImageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputAttachementsDescriptorImageInfos[2].imageView = gBufferViews[2];
        inputAttachementsDescriptorImageInfos[2].sampler = VK_NULL_HANDLE;

        inputAttachementsDescriptorImageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        inputAttachementsDescriptorImageInfos[3].imageView = gBufferViews[3];
        inputAttachementsDescriptorImageInfos[3].sampler = VK_NULL_HANDLE;

        std::vector<VkWriteDescriptorSet> inputAttachementsWrites(4);
        inputAttachementsWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inputAttachementsWrites[0].pNext = nullptr;
        inputAttachementsWrites[0].dstSet = descriptorSetsDeferredShading[i];
        inputAttachementsWrites[0].dstBinding = 6;
        inputAttachementsWrites[0].dstArrayElement = 0;
        inputAttachementsWrites[0].descriptorCount = 1;
        inputAttachementsWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputAttachementsWrites[0].pImageInfo = &(inputAttachementsDescriptorImageInfos[0]);
        inputAttachementsWrites[0].pBufferInfo = nullptr;
        inputAttachementsWrites[0].pTexelBufferView = nullptr;

        inputAttachementsWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inputAttachementsWrites[1].pNext = nullptr;
        inputAttachementsWrites[1].dstSet = descriptorSetsDeferredShading[i];
        inputAttachementsWrites[1].dstBinding = 7;
        inputAttachementsWrites[1].dstArrayElement = 0;
        inputAttachementsWrites[1].descriptorCount = 1;
        inputAttachementsWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputAttachementsWrites[1].pImageInfo = &(inputAttachementsDescriptorImageInfos[1]);
        inputAttachementsWrites[1].pBufferInfo = nullptr;
        inputAttachementsWrites[1].pTexelBufferView = nullptr;

        inputAttachementsWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inputAttachementsWrites[2].pNext = nullptr;
        inputAttachementsWrites[2].dstSet = descriptorSetsDeferredShading[i];
        inputAttachementsWrites[2].dstBinding = 8;
        inputAttachementsWrites[2].dstArrayElement = 0;
        inputAttachementsWrites[2].descriptorCount = 1;
        inputAttachementsWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputAttachementsWrites[2].pImageInfo = &(inputAttachementsDescriptorImageInfos[2]);
        inputAttachementsWrites[2].pBufferInfo = nullptr;
        inputAttachementsWrites[2].pTexelBufferView = nullptr;

        inputAttachementsWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inputAttachementsWrites[3].pNext = nullptr;
        inputAttachementsWrites[3].dstSet = descriptorSetsDeferredShading[i];
        inputAttachementsWrites[3].dstBinding = 9;
        inputAttachementsWrites[3].dstArrayElement = 0;
        inputAttachementsWrites[3].descriptorCount = 1;
        inputAttachementsWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        inputAttachementsWrites[3].pImageInfo = &(inputAttachementsDescriptorImageInfos[3]);
        inputAttachementsWrites[3].pBufferInfo = nullptr;
        inputAttachementsWrites[3].pTexelBufferView = nullptr;



        std::vector<VkWriteDescriptorSet> writeDescriptorSets;
        writeDescriptorSets.push_back(descriptorWrite);
        writeDescriptorSets.push_back(descriptorSampler);
        writeDescriptorSets.push_back(materialDescriptorWrite);
        writeDescriptorSets.push_back(decalDescriptorWrite);
        writeDescriptorSets.push_back(accelerationStructureWrite);
        if (!DISABLE_SCENE_AS)writeDescriptorSets.push_back(accelerationStructureSceneWrite);
        writeDescriptorSets.push_back(inputAttachementsWrites[0]);
        writeDescriptorSets.push_back(inputAttachementsWrites[1]);
        writeDescriptorSets.push_back(inputAttachementsWrites[2]);
        writeDescriptorSets.push_back(inputAttachementsWrites[3]);
        writeDescriptorSets.push_back(pointLightDescriptorWrite);
        writeDescriptorSets.push_back(accelerationStructurePointLightWrite);


        vkUpdateDescriptorSets(context->m_device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }
}


void DeferredRenderer::recordCommandBuffersDeferred()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    std::vector<VkClearValue> clearValues(5);
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // swap chain color
    clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // albedo
    clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // normals
    clearValues[3].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // roughness + metalicity
    clearValues[4].depthStencil = { 1.0f, 0 }; // depth & stencil

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        VkResult result = vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);
        ASSERT_VULKAN(result);

        VkRenderPassBeginInfo renderPassBeginInfo;
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.pNext = nullptr;
        renderPassBeginInfo.renderPass = renderPassDeferred;
        renderPassBeginInfo.framebuffer = framebufferDeferred[i];
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

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineDeferredGeometry);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &scene->m_vertexBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], scene->m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutDeferredGeometry, 0, 1, &(descriptorSetsDeferredGeometry[i]), 0, nullptr);
        vkCmdDrawIndexed(commandBuffers[i], scene->m_indexCount, 1, 0, 0, 0);
        if (flags & DEFERRED_DECALS)
        {
            vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineDeferredDecal);
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &scene->m_decal_vertexBuffer, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], scene->m_decal_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(commandBuffers[i],
                VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutDeferredDecal, 0, 1, &(descriptorSetsDeferredDecal[i]), 0, nullptr);
            vkCmdDrawIndexed(commandBuffers[i], scene->m_decal_indices.size(), 1, 0, 0, 0);
        }
        // Second Subpass: Shading Pass
        vkCmdNextSubpass(commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineDeferredShading);
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayoutDeferredShading, 0, 1, &(descriptorSetsDeferredShading[i]), 0, nullptr);
        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0); // Positions and UVs generated in vertex shader. (see also https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/)

        vkCmdEndRenderPass(commandBuffers[i]);

        result = vkEndCommandBuffer(commandBuffers[i]);
        ASSERT_VULKAN(result);

    }
}

void DeferredRenderer::shaderHotReload() {
    vkDeviceWaitIdle(context->m_device);
    vkDestroyShaderModule(context->m_device, shaderModuleDeferredGeometryVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleDeferredGeometryFrag, nullptr);
    if (flags & DEFERRED_DECALS)
    {
        vkDestroyShaderModule(context->m_device, shaderModuleDeferredDecalVert, nullptr);
        vkDestroyShaderModule(context->m_device, shaderModuleDeferredDecalFrag, nullptr);
    }
    vkDestroyShaderModule(context->m_device, shaderModuleDeferredShadingVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleDeferredShadingFrag, nullptr);

    vkDestroyPipelineLayout(context->m_device, pipelineLayoutDeferredGeometry, nullptr);
    if (flags & DEFERRED_DECALS)
    {
        vkDestroyPipelineLayout(context->m_device, pipelineLayoutDeferredDecal, nullptr);
    }
    vkDestroyPipelineLayout(context->m_device, pipelineLayoutDeferredShading, nullptr);

    vkDestroyPipeline(context->m_device, pipelineDeferredGeometry, nullptr);
    if (flags & DEFERRED_DECALS)
    {
        vkDestroyPipeline(context->m_device, pipelineDeferredDecal, nullptr);
    }
    vkDestroyPipeline(context->m_device, pipelineDeferredShading, nullptr);
    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffers);
    delete[] commandBuffers;
    compileShader();
    createPipelineDeferredGeometry();
    if (flags & DEFERRED_DECALS)
    {
        createPipelineDeferredDecal();
    }
    createPipelineDeferredShading();
    createCommandBuffers();
    recordCommandBuffersDeferred();
}


void DeferredRenderer::drawFrameDeferred()
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



void DeferredRenderer::createCommandBuffers() {
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


void DeferredRenderer::createSemaphores() {
    VkSemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;
    semaphoreCreateInfo.flags = 0;

    VkResult result = vkCreateSemaphore(context->m_device, &semaphoreCreateInfo, nullptr, &context->semaphoreImageAvailable);
    ASSERT_VULKAN(result);
    result = vkCreateSemaphore(context->m_device, &semaphoreCreateInfo, nullptr, &semaphoreRenderingDone);
    ASSERT_VULKAN(result);

}



void DeferredRenderer::destroyDeferredPass()
{
    gBuffer.destroy();
    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffers);
    delete[] commandBuffers;
    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        vkDestroyFramebuffer(context->m_device, framebufferDeferred[i], nullptr);
    }
    delete[] framebufferDeferred;
    vkDestroyRenderPass(context->m_device, renderPassDeferred, nullptr);
}
void DeferredRenderer::restoreDeferredPass()
{
    createRenderPassDeferred();
    createGBuffer();
    createFramebuffersDeferred();
    createCommandBuffers();

    updateDescriptorSetsDeferredShadingPass();
    if (flags & DEFERRED_DECALS)
    {
        updateDescriptorSetsDeferredDecalPass();
    }
    updateDescriptorSetsDeferredGeometryPass();

    recordCommandBuffersDeferred();
}

void DeferredRenderer::destroy()
{
    gBuffer.destroy();
    vkDestroyDescriptorSetLayout(context->m_device, descriptorSetLayoutDeferredGeometry, nullptr);
    if (flags & DEFERRED_DECALS)
    {
        vkDestroyDescriptorSetLayout(context->m_device, descriptorSetLayoutDeferredDecal, nullptr);
    }
    vkDestroyDescriptorSetLayout(context->m_device, descriptorSetLayoutDeferredShading, nullptr);

    vkDestroySemaphore(context->m_device, context->semaphoreImageAvailable, nullptr);
    vkDestroySemaphore(context->m_device, semaphoreRenderingDone, nullptr);

    vkFreeCommandBuffers(context->m_device, context->m_commandPool, context->m_amountOfImagesInSwapchain, commandBuffers);
    delete[] commandBuffers;

    for (size_t i = 0; i < context->m_amountOfImagesInSwapchain; i++)
    {
        vkDestroyFramebuffer(context->m_device, framebufferDeferred[i], nullptr);
    }
    delete[] framebufferDeferred;

    vkDestroyPipeline(context->m_device, pipelineDeferredGeometry, nullptr);
    if (flags & DEFERRED_DECALS)
    {
        vkDestroyPipeline(context->m_device, pipelineDeferredDecal, nullptr);
    }
    vkDestroyPipeline(context->m_device, pipelineDeferredShading, nullptr);

    vkDestroyRenderPass(context->m_device, renderPassDeferred, nullptr);

    vkDestroyPipelineLayout(context->m_device, pipelineLayoutDeferredGeometry, nullptr);
    if (flags & DEFERRED_DECALS)
    {
        vkDestroyPipelineLayout(context->m_device, pipelineLayoutDeferredDecal, nullptr);
    }
    vkDestroyPipelineLayout(context->m_device, pipelineLayoutDeferredShading, nullptr);

    vkDestroyShaderModule(context->m_device, shaderModuleDeferredGeometryVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleDeferredGeometryFrag, nullptr);
    if (flags & DEFERRED_DECALS)
    {
        vkDestroyShaderModule(context->m_device, shaderModuleDeferredDecalVert, nullptr);
        vkDestroyShaderModule(context->m_device, shaderModuleDeferredDecalFrag, nullptr);
    }
    vkDestroyShaderModule(context->m_device, shaderModuleDeferredShadingVert, nullptr);
    vkDestroyShaderModule(context->m_device, shaderModuleDeferredShadingFrag, nullptr);
}
