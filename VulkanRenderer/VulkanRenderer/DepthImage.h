#pragma once

#include "VulkanUtils.h"
#include <stdexcept>

using namespace VulkanUtils;

class DepthImage
{
private:
	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	bool m_created = false;

public:
	DepthImage();

	~DepthImage();

	DepthImage(const DepthImage&) = delete;
	DepthImage(DepthImage&&) = delete;
	DepthImage& operator=(const DepthImage&) = delete;
	DepthImage& operator=(DepthImage&&) = delete;


	void create(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, uint32_t width, uint32_t height);

	void destroy();

	VkImageView getImageView();

	static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

	static VkAttachmentDescription getDepthAttachmentDepthPass(VkPhysicalDevice physicalDevice);

	static VkAttachmentDescription getDepthAttachmentRenderPass(VkPhysicalDevice physicalDevice);

	static VkAttachmentDescription getDepthAttachmentDeferred(VkPhysicalDevice physicalDevice);

	static VkPipelineDepthStencilStateCreateInfo getDepthStencilStateCreateInfoOpaqueDepthPass();

	static VkPipelineDepthStencilStateCreateInfo getDepthStencilStateCreateInfoOpaqueRenderPass();
};