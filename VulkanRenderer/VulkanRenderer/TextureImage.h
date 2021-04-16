#pragma once


#include "VulkanUtils.h"
#include "stb_image.h"
#include <stdexcept>

using namespace VulkanUtils;

class TextureImage
{
private:
	int m_width;
	int m_height;
	int m_channels;
	stbi_uc* m_ppixels;
	bool m_loaded = false;
	bool m_uploaded = false;
	VkImage m_image;
	VkDeviceMemory m_imageMemory;
	VkImageView m_imageView;
	VkImageLayout m_imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	VkDevice m_device;
	VkSampler m_sampler;
	uint32_t m_mipLevels;

public:
	TextureImage();




	TextureImage(const char* path);

	~TextureImage();

	void load(const char* path);



	void upload(const VkDevice& device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, bool useSRGB);

	void changeLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImageLayout layout, VkFormat format);

	void generateMipmaps(VkDevice device, VkCommandPool commandPool, VkQueue queue);


	void writeBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer buffer);

	TextureImage(const TextureImage &) = delete;
	TextureImage(TextureImage&&) = delete;
	TextureImage& operator=(const TextureImage&) = delete;
	TextureImage& operator=(TextureImage&&) = delete;


	void destroy();


	int getHeight();

	int getWidth();

	int getChannels();

	int getSizeInBytes();

	stbi_uc* getRaw();

	VkSampler getSampler();


	VkImageView getImageView();


	static void loadImage(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
		std::vector<TextureImage*>* images, std::string imagePath, bool useSRGB);

	static void loadImages(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
		std::vector<TextureImage*>* images, std::string imageDirPath, std::vector<bool> useSRGB);

	static void loadImages(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
		std::vector<TextureImage*>* images, std::vector<std::string> imageFiles, std::vector<bool> useSRGB);



};