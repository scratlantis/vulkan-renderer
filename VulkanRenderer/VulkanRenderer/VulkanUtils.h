#pragma once
#define GLFW_INCLUDE_VULKAN
#define VK_ENABLE_BETA_EXTENSIONS
#include <GLFW\glfw3.h>
#include <vector>
#include <tiny_obj_loader.h>

// Provided by VK_KHR_ray_tracing
#define VK_LOAD(FUNCTION_NAME) PFN_##FUNCTION_NAME p##FUNCTION_NAME = (PFN_##FUNCTION_NAME) glfwGetInstanceProcAddress(instance, #FUNCTION_NAME);


#include <glm/glm.hpp>
#include <string>
#include <sstream> 
#include <filesystem>
#include <fstream>
#define ASSERT_VULKAN(val)\
            if(val!=VK_SUCCESS) {\
                __debugbreak();\
            }

#define MAX_UINT32 0xFFFFFFFF

namespace VulkanUtils {


    uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    bool isFormatSupported(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

    VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

    bool isStencilFormat(VkFormat format);

    VkCommandBuffer startSingleTimeCommandBuffer(VkDevice device, VkCommandPool commandPool);

    void endSingleTimeCommandBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);


    void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage& image, VkDeviceMemory& imageMemory);


    void createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView& imageView);



    //################ Start: Buffer creation

    void createUnallocatedBuffer(
        VkDevice device,
        VkDeviceSize deviceSize,
        VkBufferUsageFlags bufferUsageFlags,
        VkBuffer& buffer);

    void allocateBuffer(
        VkDevice device,
        VkBuffer& buffer,
        VkDeviceMemory& deviceMemory,
        VkMemoryAllocateInfo memoryAllocateInfo);

    void allocateBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkBufferUsageFlags bufferUsageFlags,
        VkBuffer& buffer,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkDeviceMemory& deviceMemory,
        VkMemoryRequirements memoryRequirements);


    void createBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize deviceSize,
        VkBufferUsageFlags bufferUsageFlags,
        VkBuffer& buffer,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkDeviceMemory& deviceMemory);


    void createBufferDedicated(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize deviceSize,
        VkBufferUsageFlags bufferUsageFlags,
        VkBuffer& buffer,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkDeviceMemory& deviceMemory);

    void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dest, VkDeviceSize size);

    
    template <typename T, typename A>
    void createBufferView(VkDevice device, VkBuffer buffer, VkFormat format, VkBufferView& bufferView, std::vector<T, A> data)
    {
        VkBufferViewCreateInfo bufferViewCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO };
        bufferViewCreateInfo.pNext = nullptr;
        bufferViewCreateInfo.flags = 0;
        bufferViewCreateInfo.buffer = buffer;
        bufferViewCreateInfo.format = format;
        bufferViewCreateInfo.offset = 0;
        bufferViewCreateInfo.range = data.size() * sizeof(T);
        VkResult result = vkCreateBufferView(device, &bufferViewCreateInfo, nullptr, &bufferView);
        ASSERT_VULKAN(result);
    }

    template <typename T, typename A>
    void createAndUploadBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue,
        VkCommandPool commandPool, std::vector<T, A> data, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& deviceMemory) {
        VkDeviceSize bufferSize = sizeof(T) * data.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferMemory);

        void* rawData;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &rawData);
        memcpy(rawData, data.data(), bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(device, physicalDevice, bufferSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceMemory);

        copyBuffer(device, commandPool, queue, stagingBuffer, buffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    template <typename T, typename A>
    void createAndUploadBufferDedicated(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue,
        VkCommandPool commandPool, std::vector<T, A> data, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& deviceMemory) {
        VkDeviceSize bufferSize = sizeof(T) * data.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferMemory);

        void* rawData;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &rawData);
        memcpy(rawData, data.data(), bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBufferDedicated(device, physicalDevice, bufferSize, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, deviceMemory);

        copyBuffer(device, commandPool, queue, stagingBuffer, buffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }


    //################ End: Buffer creation


    void changeImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);


    std::vector<std::string> listFilesInDirectory(std::string pathToDirectory);


    std::vector<char> readFile(const std::string& filename);


    uint32_t getUniformBufferOffset(uint32_t bufferSize, uint32_t index, VkPhysicalDeviceProperties deviceProperties);

    void createShaderModule(VkDevice device, const std::vector<char>& code, VkShaderModule* shaderModule);


    std::vector<std::string> split(const std::string& s, char delimiter);

    float stringToFloat(std::string s);
    float stringToInt(std::string s);


}
