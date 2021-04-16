#pragma once

#include "VulkanUtils.h"
#include "GBuffer.h"
#include "VulkanContext.h"
#include "Scene.h"
#include <iostream>
#include "Vertex.h"



enum DeferredRendererConfigurationFlags
{
    DEFERRED_DECALS = 0x00000001,
};


class DeferredRenderer
{
private:
    
    VulkanContext* context;
    Scene* scene;
    uint32_t flags;


	VkShaderModule shaderModuleDeferredGeometryVert;
	VkShaderModule shaderModuleDeferredGeometryFrag;

    VkShaderModule shaderModuleDeferredDecalVert;
    VkShaderModule shaderModuleDeferredDecalFrag;

	VkShaderModule shaderModuleDeferredShadingVert;
	VkShaderModule shaderModuleDeferredShadingFrag;


	VkPipelineLayout pipelineLayoutDeferredGeometry;
	VkPipelineLayout pipelineLayoutDeferredDecal;
	VkPipelineLayout pipelineLayoutDeferredShading;



	VkRenderPass renderPassDeferred;
	VkFramebuffer* framebufferDeferred;

	GBuffer gBuffer;

	VkPipeline pipelineDeferredGeometry;
	VkPipeline pipelineDeferredDecal;
	VkPipeline pipelineDeferredShading;


	VkDescriptorSetLayout descriptorSetLayoutDeferredGeometry;
	VkDescriptorSetLayout descriptorSetLayoutDeferredDecal;
	VkDescriptorSetLayout descriptorSetLayoutDeferredShading;


	VkDescriptorSet* descriptorSetsDeferredGeometry;
	VkDescriptorSet* descriptorSetsDeferredDecal;
	VkDescriptorSet* descriptorSetsDeferredShading;

    VkCommandBuffer* commandBuffers;
    VkSemaphore semaphoreImageAvailable;
    VkSemaphore semaphoreRenderingDone;

public:
    DeferredRenderer(VulkanContext* context, Scene* scene, uint32_t flags);
	~DeferredRenderer();

    void createRenderPassDeferred();
    void createDescriptorSetLayoutDeferredGeometry();
    void createDescriptorSetLayoutDeferredDecal();
    void createDescriptorSetLayoutDeferredShading();
    void compileShader();
    void createPipelineDeferredGeometry();
    void createPipelineDeferredDecal();
    void createPipelineDeferredShading();
    void createFramebuffersDeferred();
    void createGBuffer();
    void createDescriptorSetsDeferredGeometryPass();
    void updateDescriptorSetsDeferredGeometryPass();
    void createDescriptorSetsDeferredDecalPass();
    void updateDescriptorSetsDeferredDecalPass();
    void createDescriptorSetsDeferredShadingPass();
    void updateDescriptorSetsDeferredShadingPass();
    void recordCommandBuffersDeferred();
    void shaderHotReload();
    void drawFrameDeferred();
    void createCommandBuffers();
    void createSemaphores();
    void destroyDeferredPass();
    void restoreDeferredPass();

    void destroy();
};