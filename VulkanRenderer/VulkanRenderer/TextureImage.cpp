#pragma once


#include "VulkanUtils.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdexcept>
#include <iostream>
#include "TextureImage.h"

using namespace VulkanUtils;


TextureImage::TextureImage()
{
	m_loaded = false;
}




TextureImage::TextureImage(const char* path)
{
	load(path);
}

TextureImage::~TextureImage()
{
	destroy();
}

void TextureImage::load(const char* path)
{
	if (m_loaded)
	{
		throw std::logic_error("EasyImage was already loaded!");
	}

	m_ppixels = stbi_load(path, &m_width, &m_height, &m_channels, STBI_rgb_alpha);

	if (m_ppixels == nullptr)
	{
		throw std::invalid_argument("Could not load image, or image is corrupt!");
	}

	m_loaded = true;
}



void TextureImage::upload(const VkDevice& device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, bool useSRGB)
{
	if (!m_loaded)
	{
		throw std::logic_error("Easy Image was not loaded!");
	}

	if (m_width != m_height)
	{
		throw std::logic_error("Image height doesnt match Image width !");
	}


	m_mipLevels = std::floor(std::log2(std::max(m_width, m_height))) + 1;

	//m_mipLevels = 1;


	if (m_uploaded)
	{
		throw std::logic_error("EasyImage was already Uploaded!");
	}

	this->m_device = device;

	VkDeviceSize imageSize = getSizeInBytes();

	VkBuffer stagingBuffer;

	VkDeviceMemory stagingBufferMemory;


	VkFormat format = useSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

	createBuffer(device, physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferMemory);


	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, getRaw(), imageSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createImage(device, physicalDevice, getWidth(), getHeight(), m_mipLevels, format, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_image, m_imageMemory);

	changeLayout(device, commandPool, queue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, format);
	writeBufferToImage(device, commandPool, queue, stagingBuffer);

	generateMipmaps(device, commandPool, queue);

	//changeLayout(device, commandPool, queue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);


	createImageView(m_device, m_image, format, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels, m_imageView);






	VkSamplerCreateInfo samplerCreateInfo;
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = nullptr;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = 16;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = m_mipLevels;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;


	VkResult result = vkCreateSampler(device, &samplerCreateInfo, nullptr, &m_sampler);
	ASSERT_VULKAN(result);

	m_uploaded = true;

	stbi_image_free(m_ppixels);

}

void TextureImage::changeLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImageLayout layout, VkFormat format)
{

	changeImageLayout(device, commandPool, queue, m_image, format, this->m_imageLayout, layout, m_mipLevels);
	this->m_imageLayout = layout;

}

void TextureImage::generateMipmaps(VkDevice device, VkCommandPool commandPool, VkQueue queue) {

	VkCommandBuffer commandBuffer = startSingleTimeCommandBuffer(device, commandPool);


	VkImageMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;



	int32_t mipWidth = m_width;
	int32_t mipHeight = m_height;

	for (size_t i = 1; i < m_mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;


		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &barrier);


		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth,	mipHeight , 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);



		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);


		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;

	}

	barrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);


	endSingleTimeCommandBuffer(device, queue, commandPool, commandBuffer);


}


void TextureImage::writeBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer buffer)
{
	VkCommandBuffer commandBuffer = startSingleTimeCommandBuffer(device, commandPool);


	VkBufferImageCopy bufferImageCopy;
	bufferImageCopy.bufferOffset = 0;
	bufferImageCopy.bufferRowLength = 0;
	bufferImageCopy.bufferImageHeight = 0;
	bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferImageCopy.imageSubresource.mipLevel = 0;
	bufferImageCopy.imageSubresource.baseArrayLayer = 0;
	bufferImageCopy.imageSubresource.layerCount = 1;
	bufferImageCopy.imageOffset = { 0, 0, 0 };
	bufferImageCopy.imageExtent = { (uint32_t)getWidth(), (uint32_t)getHeight(), 1 };


	vkCmdCopyBufferToImage(commandBuffer, buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);


	endSingleTimeCommandBuffer(device, queue, commandPool, commandBuffer);
}

void TextureImage::destroy()
{
	if (m_loaded) {
		//stbi_image_free(m_ppixels);
		m_loaded = false;
	}

	if (m_uploaded)
	{
		vkDestroySampler(m_device, m_sampler, nullptr);
		vkDestroyImageView(m_device, m_imageView, nullptr);

		vkDestroyImage(m_device, m_image, nullptr);
		vkFreeMemory(m_device, m_imageMemory, nullptr);

		m_uploaded = false;
	}
}


int TextureImage::getHeight()
{
	if (!m_loaded)
	{
		throw std::logic_error("Image was not loaded!");
	}

	return m_height;
}

int TextureImage::getWidth()
{
	if (!m_loaded)
	{
		throw std::logic_error("Image was not loaded!");
	}

	return m_width;
}

int TextureImage::getChannels()
{
	if (!m_loaded)
	{
		throw std::logic_error("Image was not loaded!");
	}

	return 4;

}

int TextureImage::getSizeInBytes()
{

	if (!m_loaded)
	{
		throw std::logic_error("Image was not loaded!");
	}

	return getWidth() * getHeight() * getChannels();

}

stbi_uc* TextureImage::getRaw()
{
	if (!m_loaded)
	{
		throw std::logic_error("Image was not loaded!");
	}

	return m_ppixels;

}

VkSampler TextureImage::getSampler()
{
	if (!m_loaded)
	{
		throw std::logic_error("Image was not loaded!");
	}

	return m_sampler;
}


VkImageView TextureImage::getImageView()
{
	if (!m_loaded)
	{
		throw std::logic_error("Image was not loaded!");
	}

	return m_imageView;
}


void TextureImage::loadImage(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
	std::vector<TextureImage*>* images, std::string imagePath, bool useSRGB) {
	TextureImage* image = new TextureImage(imagePath.c_str());
	image->upload(device, physicalDevice, commandPool, queue, useSRGB);
	images->push_back(image);
}

void TextureImage::loadImages(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
	std::vector<TextureImage*>* images, std::string imageDirPath, std::vector<bool> useSRGB) {
	std::vector<std::string> imagePaths = listFilesInDirectory(imageDirPath);
	for (size_t i = 0; i < imagePaths.size(); i++)
	{
		TextureImage* image = new TextureImage(imagePaths[i].c_str());
		image->upload(device, physicalDevice, commandPool, queue, useSRGB[i]);
		images->push_back(image);
	}
}
void TextureImage::loadImages(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
	std::vector<TextureImage*>* images, std::vector<std::string> imageFiles, std::vector<bool> useSRGB) {
	for (size_t i = 0; i < imageFiles.size(); i++)
	{
		//imageFiles[i] = split(imageFiles[i], '.')[0].append("_1.png");// For Half Size
		std::cout << "Loaded Texture: " << imageFiles[i] << std::endl;
		TextureImage* image = new TextureImage(imageFiles[i].c_str());
		image->upload(device, physicalDevice, commandPool, queue, useSRGB[i]);
		images->push_back(image);
	}
}