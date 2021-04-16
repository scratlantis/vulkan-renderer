#pragma once


#include "Vertex.h"
#include "Node.h"


struct BLAS
{
    VkAccelerationStructureKHR as;
    VkDeviceMemory asMemory;
    std::vector<VkAccelerationStructureGeometryKHR> asGeometry;
    std::vector<VkAccelerationStructureCreateGeometryTypeInfoKHR> asCreateGeometryInfo;
    std::vector<VkAccelerationStructureBuildOffsetInfoKHR> asBuildOffsetInfo;

};


struct TLAS
{
    VkAccelerationStructureKHR as;
    VkDeviceMemory asMemory;
    std::vector<VkAccelerationStructureInstanceKHR> instance;

};


// Multiple blas, one tlas
struct AccelerationStructure
{
    std::vector<BLAS> bottomLevelsAs;
    TLAS topLevelAs;

    VkBuffer bottomLevelScratchBuffer;
    VkDeviceMemory bottomLevelScratchBufferMemory;

    VkBuffer topLevelScratchBuffer;
    VkDeviceMemory topLevelScratchBufferMemory;

    VkBuffer instanceScratchBuffer;
    VkDeviceMemory instanceScratchBufferMemory;
};


struct ObjInstance
{


    glm::mat4 transformMatrix;
    uint32_t objIndex;


};


class AccelerationStructureBuilder
{
private:

    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    VkBuffer m_aabbBuffer;
    VkDeviceMemory m_aabbMemory;

public:
	AccelerationStructureBuilder();
	~AccelerationStructureBuilder();



    void destroyAccelerationStructure(VkDevice device, VkInstance instance, AccelerationStructure* as);

    void decalsToAABBBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue,
        VkCommandPool commandPool, std::vector<Decal> decals, VkBuffer& aabbBuffer, VkDeviceMemory& aabbMemory);

    BLAS aabbToVkGeometryKHR(VkDevice device, VkInstance instance, size_t aabbCount, VkBuffer& aabbBuffer);

    BLAS objectToVkGeometryKHR(VkDevice device, size_t vertexCount,
        size_t indexCount, VkBuffer& vertexBuffer, VkBuffer& indexBuffer);

    void createBottomLevelAS(VkDevice device, VkInstance instance,
        VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, AccelerationStructure* as);


    VkTransformMatrixKHR getTransformMatrix(glm::mat4 mat);


    void createTopLevelAS(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, VkQueue queue, AccelerationStructure* as, std::vector<ObjInstance> objInstances);


    AccelerationStructure createFullAccelerationStructure(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, VkQueue queue, size_t vertexCount, size_t indexCount,
        VkBuffer& vertexBuffer, VkBuffer& indexBuffer, glm::mat4 transform);




    AccelerationStructure createFullAABBAccelerationStructure(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, VkQueue queue, size_t aabbCount, VkBuffer& aabbBuffer, glm::mat4 transform);



    AccelerationStructure createDecalAccelerationStructure(VkDevice device, VkInstance instance,
        VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, Node node);

    AccelerationStructure createPointLightAccelerationStructure(VkDevice device, VkInstance instance,
        VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, Node node);

    AccelerationStructure createMeshAccelerationStructure(VkDevice device, VkInstance instance,
        VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue, Node node);
};
