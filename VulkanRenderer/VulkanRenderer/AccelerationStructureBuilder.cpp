#pragma once



#include "AccelerationStructureBuilder.h"

#include "VulkanUtils.h"
#include <iostream>

using namespace VulkanUtils;

void AccelerationStructureBuilder::destroyAccelerationStructure(VkDevice device, VkInstance instance, AccelerationStructure* as)
{
    VK_LOAD(vkDestroyAccelerationStructureKHR);

    pvkDestroyAccelerationStructureKHR(device, as->topLevelAs.as, nullptr);
    vkFreeMemory(device, as->topLevelAs.asMemory, nullptr);

    for (size_t i = 0; i < as->bottomLevelsAs.size(); i++)
    {
        pvkDestroyAccelerationStructureKHR(device, as->bottomLevelsAs[i].as, nullptr);
        vkFreeMemory(device, as->bottomLevelsAs[i].asMemory, nullptr);
    }


    vkFreeMemory(device, as->topLevelScratchBufferMemory, nullptr);
    vkDestroyBuffer(device, as->topLevelScratchBuffer, nullptr);

    vkFreeMemory(device, as->bottomLevelScratchBufferMemory, nullptr);
    vkDestroyBuffer(device, as->bottomLevelScratchBuffer, nullptr);

    vkFreeMemory(device, as->instanceScratchBufferMemory, nullptr);
    vkDestroyBuffer(device, as->instanceScratchBuffer, nullptr);


}

void AccelerationStructureBuilder::decalsToAABBBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool, std::vector<Decal> decals, VkBuffer& aabbBuffer, VkDeviceMemory& aabbMemory)
{
    std::vector<VkAabbPositionsKHR> aabbs;
    for (size_t i = 0; i < decals.size(); i++)
    {
        aabbs.push_back(decals[i].aabb);
    }
    createAndUploadBufferDedicated(device, physicalDevice, queue, commandPool, aabbs, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, aabbBuffer, aabbMemory);
}

BLAS AccelerationStructureBuilder::aabbToVkGeometryKHR(VkDevice device, VkInstance instance, size_t aabbCount, VkBuffer& aabbBuffer)
{
    VK_LOAD(vkGetBufferDeviceAddressKHR);
    VkAccelerationStructureCreateGeometryTypeInfoKHR accelerationStructureGeometryTypeCreateInfo = {};
    accelerationStructureGeometryTypeCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
    accelerationStructureGeometryTypeCreateInfo.pNext = nullptr;
    accelerationStructureGeometryTypeCreateInfo.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    accelerationStructureGeometryTypeCreateInfo.maxPrimitiveCount = aabbCount;
    accelerationStructureGeometryTypeCreateInfo.indexType = VK_INDEX_TYPE_NONE_KHR;
    accelerationStructureGeometryTypeCreateInfo.maxVertexCount = 0;
    accelerationStructureGeometryTypeCreateInfo.vertexFormat = VK_FORMAT_UNDEFINED;
    accelerationStructureGeometryTypeCreateInfo.allowsTransforms = VK_FALSE;





    VkBufferDeviceAddressInfo aabbBufferDeviceAddressInfo = {};
    aabbBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    aabbBufferDeviceAddressInfo.pNext = nullptr;
    aabbBufferDeviceAddressInfo.buffer = aabbBuffer;
    VkDeviceAddress dataAddress = pvkGetBufferDeviceAddressKHR(device, &aabbBufferDeviceAddressInfo);
    VkAccelerationStructureGeometryAabbsDataKHR aabbs = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR };
    aabbs.data.deviceAddress = dataAddress;
    aabbs.stride = sizeof(VkAabbPositionsKHR);

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    accelerationStructureGeometry.geometryType = accelerationStructureGeometryTypeCreateInfo.geometryType;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.aabbs = aabbs;



    VkAccelerationStructureBuildOffsetInfoKHR accelerationStructureOffsetBuildInfo = {};
    accelerationStructureOffsetBuildInfo.primitiveCount = accelerationStructureGeometryTypeCreateInfo.maxPrimitiveCount;
    accelerationStructureOffsetBuildInfo.primitiveOffset = 0;
    accelerationStructureOffsetBuildInfo.firstVertex = 0;
    accelerationStructureOffsetBuildInfo.transformOffset = 0;



    // Currently only one Geometry per blas
    BLAS blas = {};
    blas.asGeometry.push_back(accelerationStructureGeometry);
    blas.asCreateGeometryInfo.push_back(accelerationStructureGeometryTypeCreateInfo);
    blas.asBuildOffsetInfo.push_back(accelerationStructureOffsetBuildInfo);


    return blas;
}

BLAS AccelerationStructureBuilder::objectToVkGeometryKHR(VkDevice device, size_t vertexCount, size_t indexCount, VkBuffer& vertexBuffer, VkBuffer& indexBuffer)
{

    VkAccelerationStructureCreateGeometryTypeInfoKHR accelerationStructureGeometryTypeCreateInfo = {};
    accelerationStructureGeometryTypeCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
    accelerationStructureGeometryTypeCreateInfo.pNext = nullptr;
    accelerationStructureGeometryTypeCreateInfo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR; // Use AABBs
    accelerationStructureGeometryTypeCreateInfo.maxPrimitiveCount = indexCount / 3;
    accelerationStructureGeometryTypeCreateInfo.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometryTypeCreateInfo.maxVertexCount = vertexCount;
    accelerationStructureGeometryTypeCreateInfo.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometryTypeCreateInfo.allowsTransforms = VK_FALSE;



    VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo = {};
    vertexBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    vertexBufferDeviceAddressInfo.pNext = nullptr;
    vertexBufferDeviceAddressInfo.buffer = vertexBuffer;

    VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo = {};
    indexBufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    indexBufferDeviceAddressInfo.pNext = nullptr;
    indexBufferDeviceAddressInfo.buffer = indexBuffer;



    VkAccelerationStructureGeometryTrianglesDataKHR accelerationStructureGeometryTriangelsData = {};
    accelerationStructureGeometryTriangelsData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometryTriangelsData.pNext = nullptr;
    accelerationStructureGeometryTriangelsData.vertexFormat = accelerationStructureGeometryTypeCreateInfo.vertexFormat;
    accelerationStructureGeometryTriangelsData.vertexData.deviceAddress = vkGetBufferDeviceAddress(device, &vertexBufferDeviceAddressInfo);
    accelerationStructureGeometryTriangelsData.vertexStride = sizeof(Vertex);
    accelerationStructureGeometryTriangelsData.indexType = accelerationStructureGeometryTypeCreateInfo.indexType;
    accelerationStructureGeometryTriangelsData.indexData.deviceAddress = vkGetBufferDeviceAddress(device, &indexBufferDeviceAddressInfo);
    accelerationStructureGeometryTriangelsData.transformData = {};


    VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.pNext = nullptr;
    accelerationStructureGeometry.geometryType = accelerationStructureGeometryTypeCreateInfo.geometryType;
    accelerationStructureGeometry.geometry = {};
    accelerationStructureGeometry.geometry.triangles = accelerationStructureGeometryTriangelsData;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;


    VkAccelerationStructureBuildOffsetInfoKHR accelerationStructureOffsetBuildInfo = {};
    accelerationStructureOffsetBuildInfo.primitiveCount = accelerationStructureGeometryTypeCreateInfo.maxPrimitiveCount;
    accelerationStructureOffsetBuildInfo.primitiveOffset = 0;
    accelerationStructureOffsetBuildInfo.firstVertex = 0;
    accelerationStructureOffsetBuildInfo.transformOffset = 0;







    // Currently only one Geometry per blas
    BLAS blas;
    blas.asGeometry.push_back(accelerationStructureGeometry);
    blas.asCreateGeometryInfo.push_back(accelerationStructureGeometryTypeCreateInfo);
    blas.asBuildOffsetInfo.push_back(accelerationStructureOffsetBuildInfo);


    return blas;
}

void AccelerationStructureBuilder::createBottomLevelAS(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, AccelerationStructure* as)
{
    VK_LOAD(vkCreateAccelerationStructureKHR);
    VK_LOAD(vkGetAccelerationStructureMemoryRequirementsKHR);
    VK_LOAD(vkCmdBuildAccelerationStructureKHR);
    VK_LOAD(vkBindAccelerationStructureMemoryKHR);


    uint32_t memoryTypeBits = MAX_UINT32;
    VkDeviceSize maxScratchSize = 0;

    for (size_t i = 0; i < as->bottomLevelsAs.size(); i++)
    {

        VkAccelerationStructureCreateInfoKHR accelerationStrutureCreateInfo = {};
        accelerationStrutureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStrutureCreateInfo.pNext = nullptr;
        accelerationStrutureCreateInfo.compactedSize = 0;
        accelerationStrutureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStrutureCreateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStrutureCreateInfo.maxGeometryCount = (uint32_t)as->bottomLevelsAs[i].asCreateGeometryInfo.size();
        accelerationStrutureCreateInfo.pGeometryInfos = as->bottomLevelsAs[i].asCreateGeometryInfo.data();
        accelerationStrutureCreateInfo.deviceAddress = 0;

        // Create BLAS
        VkResult result = pvkCreateAccelerationStructureKHR(device, &accelerationStrutureCreateInfo, nullptr, &(as->bottomLevelsAs[i].as));
        ASSERT_VULKAN(result);

        // ############ start: Bind & allocate blas memory
        // Find Memory
        VkAccelerationStructureMemoryRequirementsInfoKHR memInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR };
        memInfo.accelerationStructure = as->bottomLevelsAs[i].as;
        memInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
        memInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
        VkMemoryRequirements2 memReqs{ VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
        pvkGetAccelerationStructureMemoryRequirementsKHR(device, &memInfo, &memReqs);


        // Allocate Memory
        VkMemoryAllocateFlagsInfo memFlagInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memFlagInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        VkMemoryAllocateInfo memAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memAlloc.allocationSize = memReqs.memoryRequirements.size;
        memAlloc.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memReqs.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        as->bottomLevelsAs[i].asMemory;
        result = vkAllocateMemory(device, &memAlloc, nullptr, &as->bottomLevelsAs[i].asMemory);
        ASSERT_VULKAN(result);

        // Bind Memory
        VkBindAccelerationStructureMemoryInfoKHR bind{ VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR };
        bind.accelerationStructure = as->bottomLevelsAs[i].as;
        bind.memory = as->bottomLevelsAs[i].asMemory;
        bind.memoryOffset = 0;
        pvkBindAccelerationStructureMemoryKHR(device, 1, &bind);


        // ############ end: Bind & allocate blas memory



        VkAccelerationStructureMemoryRequirementsInfoKHR accelerationStructureMemoryRequirementsInfo = {};
        accelerationStructureMemoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
        accelerationStructureMemoryRequirementsInfo.pNext = nullptr;
        accelerationStructureMemoryRequirementsInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
        accelerationStructureMemoryRequirementsInfo.accelerationStructure = as->bottomLevelsAs[i].as;

        VkMemoryRequirements2 memoryRequirements = {};
        memoryRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        memoryRequirements.pNext = nullptr;
        memoryRequirements.memoryRequirements.memoryTypeBits = 0;
        memoryRequirements.memoryRequirements.alignment = 0;
        memoryRequirements.memoryRequirements.size = 0;

        // scratch
        accelerationStructureMemoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
        pvkGetAccelerationStructureMemoryRequirementsKHR(device, &accelerationStructureMemoryRequirementsInfo, &memoryRequirements);
        VkDeviceSize scratchMemorySize = memoryRequirements.memoryRequirements.size;
        memoryTypeBits &= memoryRequirements.memoryRequirements.memoryTypeBits;
        maxScratchSize = std::max(maxScratchSize, scratchMemorySize);

    }



    createBufferDedicated(device, physicalDevice, maxScratchSize,
        VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, as->bottomLevelScratchBuffer,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, as->bottomLevelScratchBufferMemory);

    VkBufferDeviceAddressInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.buffer = as->bottomLevelScratchBuffer;

    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(device, &bufferInfo);



    VkCommandBuffer cmdBuffer = startSingleTimeCommandBuffer(device, commandPool);

    for (size_t i = 0; i < as->bottomLevelsAs.size(); i++)
    {
        VkAccelerationStructureGeometryKHR* pGeometry = as->bottomLevelsAs[i].asGeometry.data();

        VkAccelerationStructureBuildGeometryInfoKHR bottomASInfo = {};
        bottomASInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        bottomASInfo.pNext = nullptr;
        bottomASInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        bottomASInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        bottomASInfo.update = VK_FALSE;
        bottomASInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        bottomASInfo.dstAccelerationStructure = as->bottomLevelsAs[i].as;
        bottomASInfo.geometryArrayOfPointers = VK_FALSE;
        bottomASInfo.geometryCount = (uint32_t)as->bottomLevelsAs[i].asGeometry.size();
        bottomASInfo.ppGeometries = &pGeometry;
        bottomASInfo.scratchData.deviceAddress = scratchAddress;


        std::vector<VkAccelerationStructureBuildOffsetInfoKHR*> pBuildOffset(as->bottomLevelsAs[i].asBuildOffsetInfo.size());
        for (size_t j = 0; j < as->bottomLevelsAs[i].asBuildOffsetInfo.size(); j++)
        {
            pBuildOffset[j] = &(as->bottomLevelsAs[i].asBuildOffsetInfo[j]);
        }

        pvkCmdBuildAccelerationStructureKHR(cmdBuffer, 1, &bottomASInfo, pBuildOffset.data());

        VkMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.pNext = nullptr;
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR; // TODO civ
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);
    }
    endSingleTimeCommandBuffer(device, queue, commandPool, cmdBuffer);

}



VkTransformMatrixKHR AccelerationStructureBuilder::getTransformMatrix(glm::mat4 mat)
{
    VkTransformMatrixKHR transform;
    transform.matrix[0][0] = mat[0][0];
    transform.matrix[0][1] = mat[1][0];
    transform.matrix[0][2] = mat[2][0];
    transform.matrix[0][3] = mat[3][0];

    transform.matrix[1][0] = mat[0][1];
    transform.matrix[1][1] = mat[1][1];
    transform.matrix[1][2] = mat[2][1];
    transform.matrix[1][3] = mat[3][1];

    transform.matrix[2][0] = mat[0][2];
    transform.matrix[2][1] = mat[1][2];
    transform.matrix[2][2] = mat[2][2];
    transform.matrix[2][3] = mat[3][2];

    return transform;
}



void AccelerationStructureBuilder::createTopLevelAS(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, AccelerationStructure* as, std::vector<ObjInstance> objInstances)
{
    VK_LOAD(vkCreateAccelerationStructureKHR);
    VK_LOAD(vkCmdBuildAccelerationStructureKHR);
    VK_LOAD(vkGetAccelerationStructureDeviceAddressKHR);
    VK_LOAD(vkGetAccelerationStructureMemoryRequirementsKHR);
    VK_LOAD(vkBindAccelerationStructureMemoryKHR);

    uint32_t instanceCount = objInstances.size();

    for (size_t i = 0; i < instanceCount; i++)
    {

        VkTransformMatrixKHR transform = getTransformMatrix(objInstances[i].transformMatrix);


        VkAccelerationStructureInstanceKHR asInstance = {};
        asInstance.transform = transform;
        asInstance.instanceCustomIndex = i;
        asInstance.mask = 0xFF;
        asInstance.instanceShaderBindingTableRecordOffset = 0;
        asInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR | VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;

        // Get blas device address
        BLAS blas = as->bottomLevelsAs[objInstances[i].objIndex];
        VkAccelerationStructureDeviceAddressInfoKHR blasDeviceAddressInfo = {};
        blasDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        blasDeviceAddressInfo.pNext = nullptr;
        blasDeviceAddressInfo.accelerationStructure = blas.as;
        asInstance.accelerationStructureReference = pvkGetAccelerationStructureDeviceAddressKHR(device, &blasDeviceAddressInfo);

        as->topLevelAs.instance.push_back(asInstance);
    }

    VkAccelerationStructureCreateGeometryTypeInfoKHR accelerationStructureGeometryTypeCreateInfo = {};
    accelerationStructureGeometryTypeCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
    accelerationStructureGeometryTypeCreateInfo.pNext = nullptr;
    accelerationStructureGeometryTypeCreateInfo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometryTypeCreateInfo.maxPrimitiveCount = as->topLevelAs.instance.size();
    accelerationStructureGeometryTypeCreateInfo.allowsTransforms = VK_TRUE;



    VkAccelerationStructureCreateInfoKHR accelerationStrutureCreateInfo = {};
    accelerationStrutureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStrutureCreateInfo.pNext = nullptr;
    accelerationStrutureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStrutureCreateInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStrutureCreateInfo.maxGeometryCount = 1;
    accelerationStrutureCreateInfo.pGeometryInfos = &accelerationStructureGeometryTypeCreateInfo;


    VkResult result = pvkCreateAccelerationStructureKHR(device, &accelerationStrutureCreateInfo, nullptr, &(as->topLevelAs.as));
    ASSERT_VULKAN(result);




    // ############ start: Bind & allocate tlas memory
        // Find Memory
    VkAccelerationStructureMemoryRequirementsInfoKHR memInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR };
    memInfo.accelerationStructure = as->topLevelAs.as;
    memInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
    memInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_KHR;
    VkMemoryRequirements2 memReqs{ VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
    pvkGetAccelerationStructureMemoryRequirementsKHR(device, &memInfo, &memReqs);


    // Allocate Memory
    VkMemoryAllocateFlagsInfo memFlagInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memFlagInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    VkMemoryAllocateInfo memAlloc{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    memAlloc.allocationSize = memReqs.memoryRequirements.size;
    memAlloc.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memReqs.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    as->topLevelAs.asMemory;
    result = vkAllocateMemory(device, &memAlloc, nullptr, &as->topLevelAs.asMemory);
    ASSERT_VULKAN(result);

    // Bind Memory
    VkBindAccelerationStructureMemoryInfoKHR bind{ VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_KHR };
    bind.accelerationStructure = as->topLevelAs.as;
    bind.memory = as->topLevelAs.asMemory;
    bind.memoryOffset = 0;
    pvkBindAccelerationStructureMemoryKHR(device, 1, &bind);


    // ############ end: Bind & allocate tlas memory



    VkAccelerationStructureMemoryRequirementsInfoKHR accelerationStructureMemoryRequirementsInfo = {};
    accelerationStructureMemoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
    accelerationStructureMemoryRequirementsInfo.pNext = nullptr;
    accelerationStructureMemoryRequirementsInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
    accelerationStructureMemoryRequirementsInfo.accelerationStructure = as->topLevelAs.as;

    VkMemoryRequirements2 memoryRequirements = {};
    memoryRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    memoryRequirements.pNext = nullptr;


    accelerationStructureMemoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
    pvkGetAccelerationStructureMemoryRequirementsKHR(device, &accelerationStructureMemoryRequirementsInfo, &memoryRequirements);
    VkDeviceSize scratchMemorySize = memoryRequirements.memoryRequirements.size;





    createBufferDedicated(device, physicalDevice, scratchMemorySize,
        VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, as->topLevelScratchBuffer,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, as->topLevelScratchBufferMemory);

    VkBufferDeviceAddressInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferInfo.buffer = as->topLevelScratchBuffer;
    bufferInfo.pNext = nullptr;

    VkDeviceAddress scratchAddress = vkGetBufferDeviceAddress(device, &bufferInfo);



    VkDeviceSize instanceDescsSizeInBytes = as->topLevelAs.instance.size() * sizeof(VkAccelerationStructureInstanceKHR);


    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(device, physicalDevice, instanceDescsSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBufferMemory);

    void* rawData;
    vkMapMemory(device, stagingBufferMemory, 0, instanceDescsSizeInBytes, 0, &rawData);
    memcpy(rawData, as->topLevelAs.instance.data(), instanceDescsSizeInBytes);
    vkUnmapMemory(device, stagingBufferMemory);

    createBufferDedicated(device, physicalDevice, instanceDescsSizeInBytes, VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, as->instanceScratchBuffer,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, as->instanceScratchBufferMemory);

    copyBuffer(device, commandPool, queue, stagingBuffer, as->instanceScratchBuffer, instanceDescsSizeInBytes);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);




    VkCommandBuffer cmdBuffer = startSingleTimeCommandBuffer(device, commandPool);

    bufferInfo.buffer = as->instanceScratchBuffer;
    VkDeviceAddress instanceAddress = vkGetBufferDeviceAddress(device, &bufferInfo);

    VkMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;

    vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0, 1, &barrier, 0, nullptr, 0, nullptr);



    // Build TLAS
    VkAccelerationStructureGeometryInstancesDataKHR geometryInstances = {};
    geometryInstances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometryInstances.pNext = nullptr;
    geometryInstances.arrayOfPointers = VK_FALSE;
    geometryInstances.data.deviceAddress = instanceAddress;


    VkAccelerationStructureGeometryDataKHR geometry = {};
    geometry.instances = geometryInstances;


    VkAccelerationStructureGeometryKHR topASGeometry = {};
    topASGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    topASGeometry.pNext = nullptr;
    topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    topASGeometry.geometry = geometry;
    topASGeometry.flags = 0;



    const VkAccelerationStructureGeometryKHR* pGeometry = &topASGeometry;
    VkAccelerationStructureBuildGeometryInfoKHR topASInfo = {};
    topASInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    topASInfo.pNext = nullptr;
    topASInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    topASInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    topASInfo.update = VK_FALSE;
    topASInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    topASInfo.dstAccelerationStructure = as->topLevelAs.as;
    topASInfo.geometryArrayOfPointers = VK_FALSE;
    topASInfo.geometryCount = 1;
    topASInfo.ppGeometries = &pGeometry;
    topASInfo.scratchData.deviceAddress = scratchAddress;




    VkAccelerationStructureBuildOffsetInfoKHR buildOffsetInfo = {};
    buildOffsetInfo.primitiveCount = (uint32_t)as->topLevelAs.instance.size();
    buildOffsetInfo.primitiveOffset = 0;
    buildOffsetInfo.firstVertex = 0;
    buildOffsetInfo.transformOffset = 0;

    const VkAccelerationStructureBuildOffsetInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

    pvkCmdBuildAccelerationStructureKHR(cmdBuffer, 1, &topASInfo, &pBuildOffsetInfo);


    endSingleTimeCommandBuffer(device, queue, commandPool, cmdBuffer);

}


AccelerationStructure AccelerationStructureBuilder::createFullAccelerationStructure(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, size_t vertexCount, size_t indexCount, VkBuffer& vertexBuffer, VkBuffer& indexBuffer, glm::mat4 transform)
{
    AccelerationStructure as;

    std::vector<ObjInstance> objInstances;


    // Currently only one Instance of Top Level Acceleration Structure
    ObjInstance testInstance = {};
    testInstance.objIndex = 0;

    // Indentity Matrix 4x4
    testInstance.transformMatrix = transform;//glm::mat4(1.0);
    objInstances.push_back(testInstance);


    // Currently only one blas
    as.bottomLevelsAs.push_back(objectToVkGeometryKHR(device, vertexCount, indexCount, vertexBuffer, indexBuffer));

    // Build blas
    vkDeviceWaitIdle(device);
    std::cout << "Create BLAS" << std::endl;
    createBottomLevelAS(device, instance, physicalDevice, commandPool, queue, &as);
    vkDeviceWaitIdle(device);
    // Build tlas
    std::cout << "Create TLAS" << std::endl;
    createTopLevelAS(device, instance, physicalDevice, commandPool, queue, &as, objInstances);
    std::cout << "Done Creating AS" << std::endl;
    vkDeviceWaitIdle(device);

    return as;
}




AccelerationStructure AccelerationStructureBuilder::createFullAABBAccelerationStructure(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, size_t aabbCount, VkBuffer& aabbBuffer, glm::mat4 transform)
{
    AccelerationStructure as = {};

    std::vector<ObjInstance> objInstances;


    // Currently only one Instance of Top Level Acceleration Structure
    ObjInstance testInstance = {};
    testInstance.objIndex = 0;

    // Indentity Matrix 4x4
    testInstance.transformMatrix = transform;//glm::mat4(1.0);
    objInstances.push_back(testInstance);


    // Currently only one blas
    as.bottomLevelsAs.push_back(aabbToVkGeometryKHR(device, instance, aabbCount, aabbBuffer));

    // Build blas
    vkDeviceWaitIdle(device);
    std::cout << "Create BLAS" << std::endl;
    createBottomLevelAS(device, instance, physicalDevice, commandPool, queue, &as);
    vkDeviceWaitIdle(device);
    // Build tlas
    std::cout << "Create TLAS" << std::endl;
    createTopLevelAS(device, instance, physicalDevice, commandPool, queue, &as, objInstances);
    std::cout << "Done Creating AS" << std::endl;
    vkDeviceWaitIdle(device);

    return as;
}



AccelerationStructure AccelerationStructureBuilder::createDecalAccelerationStructure(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, Node node)
{
    decalsToAABBBuffer(device, physicalDevice, queue, commandPool, node.m_decals, m_aabbBuffer, m_aabbMemory);

    AccelerationStructure as = createFullAABBAccelerationStructure(device, instance, physicalDevice, commandPool, queue, node.m_decals.size(), m_aabbBuffer, node.m_transformMatrix);
    vkFreeMemory(device, m_aabbMemory, nullptr);
    vkDestroyBuffer(device, m_aabbBuffer, nullptr);
    return as;
}

AccelerationStructure AccelerationStructureBuilder::createPointLightAccelerationStructure(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, Node node)
{
    std::vector<VkAabbPositionsKHR> aabbs;
    for (size_t i = 0; i < node.m_pointLights.size(); i++)
    {
        aabbs.push_back(node.m_pointLights[i].aabb);
    }
    createAndUploadBufferDedicated(device, physicalDevice, queue, commandPool, aabbs, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, m_aabbBuffer, m_aabbMemory);

    AccelerationStructure as = createFullAABBAccelerationStructure(device, instance, physicalDevice, commandPool, queue, node.m_pointLights.size(), m_aabbBuffer, node.m_transformMatrix);
    vkFreeMemory(device, m_aabbMemory, nullptr);
    vkDestroyBuffer(device, m_aabbBuffer, nullptr);
    return as;
}

AccelerationStructure AccelerationStructureBuilder::createMeshAccelerationStructure(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, Node node)
{

    // Debug
    Mesh mesh = node.m_meshs[0]; // ToDo Fix
    //std::vector<Vertex> vertices(mesh.m_vertices.size());
    //vertices = mesh.m_vertices;
    //size_t vertexCount = mesh.m_vertices.size();
    //std::vector<uint32_t> indices = mesh.m_indices;
    //size_t indexCount = mesh.m_indices.size();

    createAndUploadBuffer(device, physicalDevice, queue, commandPool, mesh.m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        m_vertexBuffer, m_vertexBufferMemory);
    createAndUploadBuffer(device, physicalDevice, queue, commandPool, mesh.m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        m_indexBuffer, m_indexBufferMemory);




    AccelerationStructure as = createFullAccelerationStructure(device, instance, physicalDevice, commandPool, queue, mesh.m_vertices.size(), mesh.m_indices.size(), m_vertexBuffer, m_indexBuffer, node.m_transformMatrix);

    vkFreeMemory(device, m_vertexBufferMemory, nullptr);
    vkDestroyBuffer(device, m_vertexBuffer, nullptr);

    vkFreeMemory(device, m_indexBufferMemory, nullptr);
    vkDestroyBuffer(device, m_indexBuffer, nullptr);

    return as;
}

AccelerationStructureBuilder::AccelerationStructureBuilder()
{
}

AccelerationStructureBuilder::~AccelerationStructureBuilder()
{
}