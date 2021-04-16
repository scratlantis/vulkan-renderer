#pragma once

#include "VulkanUtils.h"
#include "GBuffer.h"
#include "VulkanContext.h"
#include "Scene.h"
#include <iostream>
#include "Vertex.h"
#include "VisibilityBuffer.h"
#include "Cursor.h"
#include "DepthImage.h"


using namespace VulkanUtils;


class VbRenderer
{
private:

	VulkanContext* context;
	Scene* scene;


	VkShaderModule shaderModuleVbGeometryVert;
	VkShaderModule shaderModuleVbGeometryFrag;
	VkShaderModule shaderModuleVbShadingVert;
	VkShaderModule shaderModuleVbShadingFrag;


	VkPipelineLayout pipelineLayoutVbGeometry;
	VkPipelineLayout pipelineLayoutVbShading;

    VisibilityBuffer visibilityBuffer;
    DepthImage depthImage;

	VkRenderPass renderPassVb;
	VkFramebuffer* framebufferVb;


	VkPipeline pipelineVbShading;
	VkPipeline pipelineVbGeometry;


	VkDescriptorSetLayout descriptorSetLayoutVbGeometry;
	VkDescriptorSetLayout descriptorSetLayoutVbShading;


	VkDescriptorSet* descriptorSetsVbGeometry;
	VkDescriptorSet* descriptorSetsVbShading;

	VkCommandBuffer* commandBuffers;
	VkSemaphore semaphoreImageAvailable;
	VkSemaphore semaphoreRenderingDone;


public:
    VbRenderer(VulkanContext* context, Scene* scene);
	~VbRenderer();




    void createRenderPassVb();
    void createDescriptorSetLayoutVbGeometry();
    void createDescriptorSetLayoutVbShading();
    void compileShader();
    void createPipelineVbGeometry();
    void createPipelineVbShading();
    void createFramebuffersVb();
    void createVisibilityBuffer();
    void createDescriptorSetsVbGeometryPass();
    void updateDescriptorSetsVbGeometryPass();
    void createDescriptorSetsVbShadingPass();
    void updateDescriptorSetsVbShadingPass();
    void recordCommandBuffersVb();
    void shaderHotReload();
    void drawFrameVb();
    void createCommandBuffers();
    void createSemaphores();
    void destroyVbPass();
    void restoreVbPass();
    void destroy();
    void createDepthImage();

};