#pragma once
#include "VulkanUtils.h"
#include "GBuffer.h"
#include "VulkanContext.h"
#include "Scene.h"
#include <iostream>
#include "Vertex.h"
#include "DepthImage.h"

class ForwardRenderer
{
private:
	VulkanContext* context;
	Scene* scene;

public:
	ForwardRenderer(VulkanContext* context, Scene* scene);
	~ForwardRenderer();




	VkFramebuffer* framebuffers;
	VkFramebuffer depthPassFramebuffer;

	//Forward
	VkShaderModule shaderModuleVert;
	VkShaderModule shaderModuleFrag;

	VkShaderModule shaderModuleDepthPassFrag;
	VkShaderModule shaderModuleDepthPassVert;


	VkPipelineLayout pipelineLayout;
	VkPipelineLayout depthPassPipelineLayout;

	VkRenderPass renderPass;
	VkRenderPass depthPass;

	VkPipeline pipeline;
	VkPipeline depthPassPipeline;

	VkCommandBuffer* commandBuffers;
	VkCommandBuffer* commandBuffersDepthPass;

	
	VkSemaphore semaphoreRenderingDone;
	VkSemaphore semaphoreDepthPrepassDone;

	VkDescriptorSetLayout descriptorSetLayout;

	VkDescriptorSet* descriptorSets;

	DepthImage depthImage;



	void createDepthPass();
    void createRenderPass();
    void createDescriptorSetLayout();
    void compileShader();
    void createDepthPassPipeline();
    void createPipeline();
    void createFramebuffers();
    void createFramebuffersDepthPass();
    void createDepthImage();
    void createCommandBuffers();
    void createDepthPassCommandBuffers();
    void createDescriptorSets();
    void recordDepthPassCommandBuffers();
    void recordCommandBuffers();
    void createSemaphores();
    void drawFrame();
    void shaderHotReload();
    void destroyForwardPass();
    void restoreForwardPass();
    void destroy();
};


