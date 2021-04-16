#pragma once


#include "VulkanUtils.h"
#include "DepthImage.h"
#include "GBuffer.h"
#include <stdexcept>

GBuffer::GBuffer()
	{

	}

GBuffer::~GBuffer()
	{
		destroy();
	}


void GBuffer::create(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, uint32_t width, uint32_t height)
{
	if (m_created)
	{
		throw std::logic_error("GBuffer was already created!");
	}
	m_device = device;


	// Albedo
	createImage(device, physicalDevice, width, height, 1, m_format[0],
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image[0], m_imageMemory[0]);
	createImageView(device, m_image[0], m_format[0], VK_IMAGE_ASPECT_COLOR_BIT, 1, m_imageView[0]);
	// Normals
	createImage(device, physicalDevice, width, height, 1, m_format[1],
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image[1], m_imageMemory[1]);
	createImageView(device, m_image[1], m_format[1], VK_IMAGE_ASPECT_COLOR_BIT, 1, m_imageView[1]);
	// Roughness + Metalicity
	createImage(device, physicalDevice, width, height, 1, m_format[2],
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image[2], m_imageMemory[2]);
	createImageView(device, m_image[2], m_format[2], VK_IMAGE_ASPECT_COLOR_BIT, 1, m_imageView[2]);


	VkFormat depthFormat = DepthImage::findDepthFormat(physicalDevice);

	createImage(device, physicalDevice, width, height, 1, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image[3], m_imageMemory[3]);
	createImageView(device, m_image[3], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1, m_imageView[3]);

	m_created = true;
}

void GBuffer::destroy()
{
	if (m_created)
	{
		for (size_t i = 0; i < m_image.size(); i++)
		{
			vkDestroyImageView(m_device, m_imageView[i], nullptr);
			vkDestroyImage(m_device, m_image[i], nullptr);
			vkFreeMemory(m_device, m_imageMemory[i], nullptr);
			m_created = false;

			m_image[i] = VK_NULL_HANDLE;
			m_imageMemory[i] = VK_NULL_HANDLE;
			m_imageView[i] = VK_NULL_HANDLE;
		}
		m_device = VK_NULL_HANDLE;

	}
}

std::vector<VkImageView> GBuffer::getImageViews()
{
	return m_imageView;
}


std::vector<VkAttachmentDescription> GBuffer::getAttachmentDescriptors(VkPhysicalDevice physicalDevice)
{
	std::vector <VkAttachmentDescription> attachementDescriptors(4);


	// albedo
	attachementDescriptors[0].flags = 0;
	attachementDescriptors[0].format = m_format[0];
	attachementDescriptors[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachementDescriptors[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachementDescriptors[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachementDescriptors[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachementDescriptors[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachementDescriptors[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachementDescriptors[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// normals
	attachementDescriptors[1].flags = 0;
	attachementDescriptors[1].format = m_format[1];
	attachementDescriptors[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachementDescriptors[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachementDescriptors[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachementDescriptors[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachementDescriptors[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachementDescriptors[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachementDescriptors[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Roughness + Metalicity
	attachementDescriptors[2].flags = 0;
	attachementDescriptors[2].format = m_format[2];
	attachementDescriptors[2].samples = VK_SAMPLE_COUNT_1_BIT;
	attachementDescriptors[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachementDescriptors[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachementDescriptors[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachementDescriptors[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachementDescriptors[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachementDescriptors[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// depth
	attachementDescriptors[3] = DepthImage::getDepthAttachmentDeferred(physicalDevice);

	return attachementDescriptors;
}