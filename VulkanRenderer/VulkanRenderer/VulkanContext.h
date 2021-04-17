#pragma once

#include "ShaderFeatureFlags.h"
#include "PerFrameConstants.h"
#include "VulkanUtils.h"



// You may need to modify
#define VULKAN_PATH "C:\\VulkanSDK\\1.2.141.2\\" // Path to Vulkan installation
#define DEVICE_INDEX 0 // Device index of your Nvidia Graphics Card


struct VulkanContext
{
	VkDevice m_device;
	VkPhysicalDevice m_physicalDevice;
	VkInstance m_instance;
	VkSurfaceKHR m_surface;
	GLFWwindow* m_window;
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	VkFence* m_fences;
	uint32_t m_imageRenderIndex = 0;
	uint32_t m_imageUpdateIndex = 0;
	PerFrameConstants m_perFrameConstants;
	const VkFormat m_colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	uint32_t m_width = 400;
	uint32_t m_height = 300;
	uint32_t m_amountOfImagesInSwapchain = 0;
	VkImageView* m_imageViews;
	VkQueue m_queue;
	VkCommandPool m_commandPool;
	VkDescriptorPool m_descriptorPool;
	VkBuffer* m_uniformBuffers;
	VkDeviceMemory* m_uniformBufferMemory;
	VkPhysicalDeviceProperties m_deviceProperties;
	uint32_t shaderFeatureFlags = 0x00000000;
	VkSemaphore semaphoreImageAvailable;
	VulkanContext(){}
	VulkanContext(VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance, VkCommandPool commandPool, VkQueue queue)
		: m_device(device), m_physicalDevice(physicalDevice), m_instance(instance){}
};