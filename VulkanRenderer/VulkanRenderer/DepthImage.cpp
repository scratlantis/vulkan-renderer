#pragma once

#include "VulkanUtils.h"
#include "DepthImage.h"
#include <stdexcept>

using namespace VulkanUtils;


DepthImage::DepthImage()
	{

	}

DepthImage::~DepthImage()
	{
		destroy();
	}


void DepthImage::create(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, uint32_t width, uint32_t height)
{
	if (m_created)
	{
		throw std::logic_error("DepthImage was already created!");
	}
	m_device = device;

	VkFormat depthFormat = findDepthFormat(physicalDevice);
	createImage(device, physicalDevice, width, height, 1, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory);

	createImageView(device, m_image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, m_imageView);

	changeImageLayout(device, commandPool, queue, m_image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

	m_created = true;

}

void DepthImage::destroy()
{
	if (m_created)
	{
		vkDestroyImageView(m_device, m_imageView, nullptr);
		vkDestroyImage(m_device, m_image, nullptr);
		vkFreeMemory(m_device, m_imageMemory, nullptr);
		m_created = false;

		m_image = VK_NULL_HANDLE;
		m_imageMemory = VK_NULL_HANDLE;
		m_imageView = VK_NULL_HANDLE;
		m_device = VK_NULL_HANDLE;

	}
}

VkImageView DepthImage::getImageView()
{
	return m_imageView;
}

VkFormat DepthImage::findDepthFormat(VkPhysicalDevice physicalDevice)
{
	std::vector<VkFormat> possibleFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT };
	return findSupportedFormat(physicalDevice, possibleFormats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}



VkAttachmentDescription DepthImage::getDepthAttachmentDepthPass(VkPhysicalDevice physicalDevice)
{
	VkAttachmentDescription depthAttachement;
	depthAttachement.flags = 0;
	depthAttachement.format = findDepthFormat(physicalDevice);
	depthAttachement.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachement.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachement.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachement.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachement.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachement.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; //VK_IMAGE_LAYOUT_UNDEFINED
	depthAttachement.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	return depthAttachement;
}

VkAttachmentDescription DepthImage::getDepthAttachmentRenderPass(VkPhysicalDevice physicalDevice)
{
	VkAttachmentDescription depthAttachement;
	depthAttachement.flags = 0;
	depthAttachement.format = findDepthFormat(physicalDevice);
	depthAttachement.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachement.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachement.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachement.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachement.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachement.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; //VK_IMAGE_LAYOUT_UNDEFINED
	depthAttachement.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	return depthAttachement;
}

VkAttachmentDescription DepthImage::getDepthAttachmentDeferred(VkPhysicalDevice physicalDevice)
{
	VkAttachmentDescription depthAttachement;
	depthAttachement.flags = 0;
	depthAttachement.format = findDepthFormat(physicalDevice);
	depthAttachement.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachement.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachement.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachement.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachement.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachement.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachement.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	return depthAttachement;
}


VkPipelineDepthStencilStateCreateInfo DepthImage::getDepthStencilStateCreateInfoOpaqueDepthPass()
{
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.pNext = nullptr;
	depthStencilStateCreateInfo.flags = 0;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;


	return depthStencilStateCreateInfo;

}

VkPipelineDepthStencilStateCreateInfo DepthImage::getDepthStencilStateCreateInfoOpaqueRenderPass()
{
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.pNext = nullptr;
	depthStencilStateCreateInfo.flags = 0;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;// VK_TRUE
	depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE; // because we use prepass
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_EQUAL; // because we use prepass
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;


	return depthStencilStateCreateInfo;

}