#pragma once

#include "VulkanUtils.h"


namespace VulkanUtils {

    uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties); // TODO civ
        for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        __debugbreak();
        throw std::runtime_error("Found no correct memory type!");
    }


    bool isFormatSupported(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

        if (tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & featureFlags) == featureFlags)
        {
            return true;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & featureFlags) == featureFlags)
        {
            return true;
        }
        return false;
    }


    VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
    {
        for (VkFormat format : formats)
        {
            if (isFormatSupported(physicalDevice, format, tiling, featureFlags))
            {
                return format;
            }
        }

        throw std::runtime_error("No supported format found!");
    }

    bool isStencilFormat(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }








    VkCommandBuffer startSingleTimeCommandBuffer(VkDevice device, VkCommandPool commandPool) {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo;
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.pNext = nullptr;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = 1;


        VkCommandBuffer commandBuffer;
        VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
        ASSERT_VULKAN(result);

        VkCommandBufferBeginInfo commandBufferBeginInfo;
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pNext = nullptr;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
        ASSERT_VULKAN(result);

        return commandBuffer;
    }


    void endSingleTimeCommandBuffer(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
    {
        VkResult result = vkEndCommandBuffer(commandBuffer);
        ASSERT_VULKAN(result);

        VkSubmitInfo submitInfo;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = nullptr;

        result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        ASSERT_VULKAN(result);
        result = vkQueueWaitIdle(queue);
        ASSERT_VULKAN(result);
        //vkDeviceWaitIdle(device);

        //std::cout << "start: Freeing single time command buffer!" << std::endl;
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        //std::cout << "end: Freeing single time command buffer!" << std::endl;
    }


    void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkImage& image, VkDeviceMemory& imageMemory)
    {
        VkImageCreateInfo imageCreateInfo;
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext = nullptr;
        imageCreateInfo.flags = 0;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.usage = usageFlags;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.queueFamilyIndexCount = 0;
        imageCreateInfo.pQueueFamilyIndices = nullptr;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;


        VkResult result = vkCreateImage(device, &imageCreateInfo, nullptr, &image);
        ASSERT_VULKAN(result);


        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(device, image, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo;
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.pNext = nullptr;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, propertyFlags);

        result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &imageMemory);
        ASSERT_VULKAN(result);

        vkBindImageMemory(device, image, imageMemory, 0);
    }


    void createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageView& imageView)
    {
        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);
        ASSERT_VULKAN(result);
    }



    //################ Start: Buffer creation

    void createUnallocatedBuffer(
        VkDevice device,
        VkDeviceSize deviceSize,
        VkBufferUsageFlags bufferUsageFlags,
        VkBuffer& buffer)
    {
        VkBufferCreateInfo bufferCreateInfo;
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext = nullptr;
        bufferCreateInfo.flags = 0;
        bufferCreateInfo.size = deviceSize;
        bufferCreateInfo.usage = bufferUsageFlags;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferCreateInfo.queueFamilyIndexCount = 0;
        bufferCreateInfo.pQueueFamilyIndices = nullptr;

        VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
        ASSERT_VULKAN(result);
    }

    void allocateBuffer(
        VkDevice device,
        VkBuffer& buffer,
        VkDeviceMemory& deviceMemory,
        VkMemoryAllocateInfo memoryAllocateInfo)
    {
        //VkMemoryRequirements memoryRequirements = {};
        //vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        VkResult result = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &deviceMemory);
        ASSERT_VULKAN(result);
    }

    void allocateBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkBufferUsageFlags bufferUsageFlags,
        VkBuffer& buffer,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkDeviceMemory& deviceMemory,
        VkMemoryRequirements memoryRequirements)
    {
        VkMemoryAllocateInfo memoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags);

        VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
        memoryAllocateFlagsInfo.pNext = nullptr;
        memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        memoryAllocateFlagsInfo.deviceMask = 0;


        if (bufferUsageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
        }
        else
        {
            memoryAllocateInfo.pNext = nullptr;
        }
        allocateBuffer(device, buffer, deviceMemory, memoryAllocateInfo);
    }


    void createBuffer(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize deviceSize,
        VkBufferUsageFlags bufferUsageFlags,
        VkBuffer& buffer,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkDeviceMemory& deviceMemory)
    {
        // Create Buffer
        createUnallocatedBuffer(device, deviceSize, bufferUsageFlags, buffer);
        VkMemoryRequirements memoryRequirements = {};
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);
        // Allocate Buffer
        allocateBuffer(device, physicalDevice, bufferUsageFlags, buffer, memoryPropertyFlags, deviceMemory, memoryRequirements);
        // Bind Buffer
        vkBindBufferMemory(device, buffer, deviceMemory, 0);
    }


    void createBufferDedicated(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkDeviceSize deviceSize,
        VkBufferUsageFlags bufferUsageFlags,
        VkBuffer& buffer,
        VkMemoryPropertyFlags memoryPropertyFlags,
        VkDeviceMemory& deviceMemory)
    {
        // Create Buffer
        createUnallocatedBuffer(device, deviceSize, bufferUsageFlags, buffer);
        // Allocate Buffer
        VkMemoryRequirements2           memReqs{ VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
        VkMemoryDedicatedRequirements   dedicatedRegs{ VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
        VkBufferMemoryRequirementsInfo2 bufferReqs{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };
        bufferReqs.buffer = buffer;
        memReqs.pNext = &dedicatedRegs;
        vkGetBufferMemoryRequirements2(device, &bufferReqs, &memReqs);
        allocateBuffer(device, physicalDevice, bufferUsageFlags, buffer, memoryPropertyFlags, deviceMemory, memReqs.memoryRequirements);
        // Bind Buffer
        vkBindBufferMemory(device, buffer, deviceMemory, 0);
    }

    void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer src, VkBuffer dest, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = startSingleTimeCommandBuffer(device, commandPool);
        VkBufferCopy bufferCopy;
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = 0;
        bufferCopy.size = size;
        vkCmdCopyBuffer(commandBuffer, src, dest, 1, &bufferCopy);
        endSingleTimeCommandBuffer(device, queue, commandPool, commandBuffer);
    }

    



    //################ End: Buffer creation


    void changeImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
    {
        VkCommandBuffer commandBuffer = startSingleTimeCommandBuffer(device, commandPool);
        VkPipelineStageFlagBits srcStage;
        VkPipelineStageFlagBits dstStage;

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext = nullptr;
        if (oldLayout == VK_IMAGE_LAYOUT_PREINITIALIZED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_HOST_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; //civ
        }
        else
        {
            throw std::invalid_argument("Layout transition not yet supported!");
        }

        imageMemoryBarrier.oldLayout = oldLayout;
        imageMemoryBarrier.newLayout = newLayout;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.image = image;
        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (isStencilFormat(format))
            {
                imageMemoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }

        }
        else
        {
            imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 1;


        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage,
            0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);


        endSingleTimeCommandBuffer(device, queue, commandPool, commandBuffer);
    }


    std::vector<std::string> listFilesInDirectory(std::string pathToDirectory)
    {
        std::vector<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(pathToDirectory))
            files.push_back(entry.path().string());
        return files;
    }


    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if (file) {
            size_t fileSize = (size_t)file.tellg();
            std::vector<char> fileBuffer(fileSize);
            file.seekg(0);
            file.read(fileBuffer.data(), fileSize);
            file.close();
            return fileBuffer;
        }
        else
        {
            throw std::runtime_error("Failed to open file!!!");
        }
    }


    uint32_t getUniformBufferOffset(uint32_t bufferSize, uint32_t index, VkPhysicalDeviceProperties deviceProperties)
    {
        uint32_t allignement = deviceProperties.limits.minUniformBufferOffsetAlignment;
        uint32_t offset = 0;
        for (size_t i = 0; i < index; i++)
        {
            offset += bufferSize;
            if (offset % allignement != 0)
            {
                offset += allignement - (offset % allignement);
            }

        }
        return offset;
    }

    void createShaderModule(VkDevice device, const std::vector<char>& code, VkShaderModule* shaderModule) {
        VkShaderModuleCreateInfo shaderCreateInfo;
        shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCreateInfo.pNext = nullptr;
        shaderCreateInfo.flags = 0;
        shaderCreateInfo.codeSize = code.size();
        shaderCreateInfo.pCode = (uint32_t*)code.data();

        VkResult result = vkCreateShaderModule(device, &shaderCreateInfo, nullptr, shaderModule);
        ASSERT_VULKAN(result);

    }


    std::vector<std::string> split(const std::string& s, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter))
        {
            tokens.push_back(token);
        }
        return tokens;
    }

    float stringToFloat(std::string s)
    {

        if (s.find_first_of(",") != std::string::npos)
            s.replace(s.find_first_of(","), 1, ".");
        float out;
        try
        {
            out = std::stof(s);
        }
        catch (const std::invalid_argument)
        {
            out = 1.0f;
        }
        return out;
    }
    float stringToInt(std::string s)
    {
        int out;
        try
        {
            out = std::stoi(s);
        }
        catch (const std::invalid_argument)
        {
            out = 1;
        }
        return out;
    }
}


