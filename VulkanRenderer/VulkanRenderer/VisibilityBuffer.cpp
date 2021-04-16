#pragma once


#include "VulkanUtils.h"
#include "VisibilityBuffer.h"
#include <stdexcept>



// VisibilityBuffer Layout:		|		R8		|		G8		|		B8		|		A8		|		= 32 Bit
// -------------------------------------------------------------------------------------------------
//								|Inst-ID 8 Bit	|		Triangle ID 24 Bit						|
//		= 32 Bit (4 Bytes) total. Instances x Triangle: 256 x 16M


using namespace VulkanUtils;

VisibilityBuffer::VisibilityBuffer()
	{

	}

VisibilityBuffer::~VisibilityBuffer()
	{
		destroy();
	}


void VisibilityBuffer::create(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, uint32_t width, uint32_t height)
{
	if (m_created)
	{
		throw std::logic_error("Visibility Buffer was already created!");
	}
	m_device = device;

	createImage(device, physicalDevice, width, height, 1, m_format,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory);
	createImageView(device, m_image, VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT, 1, m_imageView);
	m_created = true;
}

void VisibilityBuffer::destroy()
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

VkImageView VisibilityBuffer::getImageView()
{
	return m_imageView;
}




VkAttachmentDescription VisibilityBuffer::getAttachmentDescriptor(VkPhysicalDevice physicalDevice)
{
	VkAttachmentDescription attachementDescriptors;
	attachementDescriptors.flags = 0;
	attachementDescriptors.format = m_format;
	attachementDescriptors.samples = VK_SAMPLE_COUNT_1_BIT;
	attachementDescriptors.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachementDescriptors.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachementDescriptors.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachementDescriptors.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachementDescriptors.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachementDescriptors.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	return attachementDescriptors;
}