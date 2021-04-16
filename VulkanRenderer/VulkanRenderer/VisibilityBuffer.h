#pragma once


#include "VulkanUtils.h"
#include <stdexcept>



// VisibilityBuffer Layout:		|		R8		|		G8		|		B8		|		A8		|		= 32 Bit
// -------------------------------------------------------------------------------------------------
//								|Inst-ID 8 Bit	|		Triangle ID 24 Bit						|
//		= 32 Bit (4 Bytes) total. Instances x Triangle: 256 x 16M


class VisibilityBuffer
{
private:
	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
	VkImageView m_imageView = VK_NULL_HANDLE;
	VkDevice m_device = VK_NULL_HANDLE;
	const VkFormat m_format = VK_FORMAT_R32_UINT;
	bool m_created = false;

public:
	VisibilityBuffer();

	~VisibilityBuffer();

	VisibilityBuffer(const VisibilityBuffer&) = delete;
	VisibilityBuffer(VisibilityBuffer&&) = delete;
	VisibilityBuffer& operator=(const VisibilityBuffer&) = delete;
	VisibilityBuffer& operator=(VisibilityBuffer&&) = delete;


	void create(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, uint32_t width, uint32_t height);

	void destroy();

	VkImageView getImageView();



	VkAttachmentDescription getAttachmentDescriptor(VkPhysicalDevice physicalDevice);
};