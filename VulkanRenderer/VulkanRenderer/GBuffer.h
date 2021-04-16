#pragma once


#include "VulkanUtils.h"
#include <stdexcept>


// New layout
// GBuffer Layout:	|		R8		|		G8		|		B8		|		A8		|		= 32 Bit
// ----------------------------------------------------------------------------------
//					|			ALBEDO (Diffuse color)				|	-			| <- alpha
//					|			Normal xyz	3*10					| -				| <- alpha
//					|	 Roughness	| Metallicity	|



// separate
//					|					Depth 24 Bit + 8 bit stencil				|
//		= 96 Bit (12 Bytes) total

class GBuffer
{
private:
	std::vector <VkImage> m_image = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
	std::vector <VkDeviceMemory> m_imageMemory = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
	std::vector <VkImageView> m_imageView = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
	VkDevice m_device = VK_NULL_HANDLE;
	const VkFormat m_format[3] = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_R8G8_SNORM};// TODO civ
	bool m_created = false;

public:
	GBuffer();
	~GBuffer();

	GBuffer(const GBuffer&) = delete;
	GBuffer(GBuffer&&) = delete;
	GBuffer& operator=(const GBuffer&) = delete;
	GBuffer& operator=(GBuffer&&) = delete;


	void create(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, uint32_t width, uint32_t height);

	void destroy();

	std::vector<VkImageView> getImageViews();


	std::vector<VkAttachmentDescription> getAttachmentDescriptors(VkPhysicalDevice physicalDevice);
};