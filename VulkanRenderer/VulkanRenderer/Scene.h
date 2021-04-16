#pragma once

#include "VulkanUtils.h"
#include "PointLightLoader.h"
#include "MeshLoader.h"
#include "DecalLoader.h"
#include "PointLightLoader.h"
#include "AccelerationStructureBuilder.h"


#define VERTEX_ATTRIBUTE_COUNT 6
#define UNIFORM_BUFFER_COUNT 5

#define DISABLE_SCENE_AS false


struct ResourceLoadInfo
{
    const char* objPath;
    const char* mtlBaseDir;
    const char* texturePath;
};





enum SCENE_LOAD_FLAGS
{
    MESH_LOADED                     = 0x00000001,
    MATERIALS_LOADED                = 0x00000002,
    VERTEX_BUFFER_LOADED            = 0x00000004,
    INDEX_BUFFER_LOADED             = 0x00000008,
    VERTEX_ATTRIBUTE_BUFFER_LOADED  = 0x00000020,
    VERTEX_ATTRIBUTE_LIST_CREATED   = 0x00000040,
    ACCELERATION_STRUCTURE_LOADED   = 0x00000080,
    DECAL_VERTEX_BUFFER_LOADED      = 0x00000100,
    DECAL_INDEX_BUFFER_LOADED       = 0x00000200,
};


// Vertex Attribute Buffers for Visibility Buffer Rendering
struct VertexAttributeBuffers
{
	union {
		struct {
			VkBuffer positionBuffer;
			VkBuffer normalBuffer;
			VkBuffer uvCoordBuffer;
			VkBuffer tangentBuffer;
			VkBuffer bitangentBuffer;
			VkBuffer matIdxBuffer;
		};
		VkBuffer buffers[VERTEX_ATTRIBUTE_COUNT];
	};
	union {
		struct {
			VkDeviceMemory positionBufferMemory;
			VkDeviceMemory normalBufferMemory;
			VkDeviceMemory uvCoordBufferMemory;
			VkDeviceMemory tangentBufferMemory;
			VkDeviceMemory bitangentBufferMemory;
			VkDeviceMemory matIdxBufferMemory;
		};
		VkDeviceMemory memory[VERTEX_ATTRIBUTE_COUNT];
	};
	union {
		struct {
			VkBufferView positionView;
			VkBufferView normalView;
			VkBufferView uvCoordView;
			VkBufferView tangentView;
			VkBufferView bitangentView;
			VkBufferView matIdxView;
		};
		VkBufferView views[VERTEX_ATTRIBUTE_COUNT];
	};
};
struct VertexAttributeLists
{
	std::vector<glm::vec3> pos;
	std::vector<glm::vec2> uvCoord;
	std::vector<glm::vec3> normal;
	std::vector<glm::vec3> tangent;
	std::vector<glm::vec3> bitangent;
	std::vector<glm::uint> matIdx;
};



class Scene
{

private:
    void createVertexAttributeLists();
    VulkanContext* m_context;
    ResourceLoadInfo m_sceneInfo;
    MeshLoader* m_meshLoader;
    MaterialLoader* m_materialLoader;
    DecalLoader* m_decalLoader;
    PointLightLoader* m_pointLightLoader;
    AccelerationStructureBuilder* m_asBuilder;
    uint32_t m_loadStatus;

public:

    




    VertexAttributeLists m_vertexAttributeLists;
    Scene(VulkanContext* context, ResourceLoadInfo sceneInfo, MeshLoader* meshLoader, MaterialLoader* materialLoader, DecalLoader* decalLoader, PointLightLoader* pointLightLoader, AccelerationStructureBuilder* asBuilder) :
        m_context(context), m_sceneInfo(sceneInfo), m_meshLoader(meshLoader), m_materialLoader(materialLoader), m_decalLoader(decalLoader), m_pointLightLoader(pointLightLoader), m_asBuilder(asBuilder)
    {
        m_loadStatus = 0x00000000;
    }
	~Scene();


    VertexAttributeBuffers m_vertexAttributeBuffers;
	uint32_t m_materialCount;
	uint32_t m_textureCount;
	uint32_t m_decalCount;
	uint32_t m_pointLightCount;
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
	std::vector<TextureImage*> m_images;
	AccelerationStructure m_decal_as;
	AccelerationStructure m_scene_as;
	AccelerationStructure m_point_light_as;
	VkBuffer m_vertexBuffer;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_vertexBufferDeviceMemory;
	VkDeviceMemory m_indexBufferDeviceMemory;
	std::vector<Vertex>* m_vertices;
	std::vector<uint32_t>* m_indices;


    VkBuffer m_decal_vertexBuffer;
    VkBuffer m_decal_indexBuffer;
    VkDeviceMemory m_decal_vertexBufferDeviceMemory;
    VkDeviceMemory m_decal_indexBufferDeviceMemory;
    std::vector<Vertex> m_decal_vertices = {};
    std::vector<uint32_t> m_decal_indices = {};


	Node m_rootNode;

    void loadMaterials();

    void loadMesh();

    void createVertexBuffer();
    void createVertexAttributeBuffersBuffers();

    void createIndexBuffer();

    void createDecalVertexBuffer();

    void createDecalIndexBuffer();

    void recreateDecalAccelerationStructure();
    void recreateLightAccelerationStructure();

    void createAccelerationStructure();

    void destroy();
};

