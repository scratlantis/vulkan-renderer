#pragma once

#include "VulkanUtils.h"
#include "PointLightLoader.h"
#include "VulkanContext.h"
#include "AccelerationStructureBuilder.h"
#include "Scene.h"


using namespace VulkanUtils;


void Scene::createVertexAttributeLists()
{
    if (!(m_loadStatus & VERTEX_ATTRIBUTE_LIST_CREATED))
    {
        uint32_t size = (*m_indices).size();
        m_vertexAttributeLists.pos = std::vector<glm::vec3>(size);
        m_vertexAttributeLists.uvCoord = std::vector<glm::vec2>(size);
        m_vertexAttributeLists.normal = std::vector<glm::vec3>(size);
        m_vertexAttributeLists.tangent = std::vector<glm::vec3>(size);
        m_vertexAttributeLists.bitangent = std::vector<glm::vec3>(size);
        m_vertexAttributeLists.matIdx = std::vector<glm::uint>(size);
        for (size_t i = 0; i < size; i++)
        {
            m_vertexAttributeLists.pos[i] = (*m_vertices)[(*m_indices)[i]].pos;
            m_vertexAttributeLists.uvCoord[i] = (*m_vertices)[(*m_indices)[i]].uvCoord;
            m_vertexAttributeLists.normal[i] = (*m_vertices)[(*m_indices)[i]].normal;
            m_vertexAttributeLists.tangent[i] = (*m_vertices)[(*m_indices)[i]].tangent;
            m_vertexAttributeLists.bitangent[i] = (*m_vertices)[(*m_indices)[i]].bitangent;
            m_vertexAttributeLists.matIdx[i] = (*m_vertices)[(*m_indices)[i]].matIdx;
        }
        m_loadStatus |= VERTEX_ATTRIBUTE_LIST_CREATED;
    }
}


void  Scene::loadMaterials() {
    if (!(m_loadStatus & MATERIALS_LOADED))
    {
        m_materialLoader->uploadMaterials(m_context->m_device, m_context->m_physicalDevice, m_context->m_commandPool, m_context->m_queue, &m_images);
        m_loadStatus |= MATERIALS_LOADED;
    }
}

void  Scene::loadMesh() {
    if (!(m_loadStatus & MESH_LOADED))
    {
        //Set Model matrix
        m_rootNode.m_transformMatrix = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f)), glm::vec3(0.0f, 0.0f, 0.0f));
        m_meshLoader->setMaterialLoader(m_materialLoader);
        m_meshLoader->setDecalLoader(m_decalLoader);
        m_meshLoader->setPointLightLoader(m_pointLightLoader);
        m_meshLoader->createMesh(m_sceneInfo.objPath, m_sceneInfo.mtlBaseDir, m_sceneInfo.texturePath, m_rootNode);
        m_vertices = &m_rootNode.m_meshs[0].m_vertices;
        m_indices = &m_rootNode.m_meshs[0].m_indices;
        m_materialCount = m_materialLoader->getMaterialCount();
        m_textureCount = m_materialLoader->getTextureCount();
        m_pointLightCount = m_rootNode.m_pointLights.size();
        m_decalCount = m_rootNode.m_decals.size();
        m_vertexCount = (*m_vertices).size();
        m_indexCount = (*m_indices).size();

        auto decalSortRule = [](Decal const& a, Decal const& b) -> bool
        {
            return (a.channel == b.channel) ? a.weight < b.weight : a.channel < b.channel;
        };
        std::sort(m_rootNode.m_decals.begin(), m_rootNode.m_decals.end(), decalSortRule);

        for (size_t i = 0; i < m_rootNode.m_decals.size(); i++)
        {
            uint32_t offset = m_decal_vertices.size();
            for (size_t j = 0; j < m_rootNode.m_decals[i].indices.size(); j++)
            {
                m_decal_indices.push_back(m_rootNode.m_decals[i].indices[j] + offset);
            }
            for (size_t j = 0; j < m_rootNode.m_decals[i].vertices.size(); j++)
            {
                Vertex vert = m_rootNode.m_decals[i].vertices[j];
                vert.matIdx = i; // Use for decal index
                m_decal_vertices.push_back(vert);
            }
        }


        m_loadStatus |= MESH_LOADED;
    }
}

void  Scene::createVertexBuffer() {
    if (!(m_loadStatus & VERTEX_BUFFER_LOADED))
    {
        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, *m_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_vertexBuffer, m_vertexBufferDeviceMemory);
        m_loadStatus |= VERTEX_BUFFER_LOADED;
    }
}
void  Scene::createVertexAttributeBuffersBuffers()
{
    if (!(m_loadStatus & VERTEX_ATTRIBUTE_BUFFER_LOADED))
    {
        createVertexAttributeLists();
        //VertexAttributeLists vertexLists = m_vertexAttributeLists;
        uint32_t vertexCount = (*m_vertices).size();
        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, m_vertexAttributeLists.pos,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, m_vertexAttributeBuffers.positionBuffer, m_vertexAttributeBuffers.positionBufferMemory);

        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, m_vertexAttributeLists.normal,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, m_vertexAttributeBuffers.normalBuffer, m_vertexAttributeBuffers.normalBufferMemory);

        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, m_vertexAttributeLists.uvCoord,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, m_vertexAttributeBuffers.uvCoordBuffer, m_vertexAttributeBuffers.uvCoordBufferMemory);

        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, m_vertexAttributeLists.tangent,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, m_vertexAttributeBuffers.tangentBuffer, m_vertexAttributeBuffers.tangentBufferMemory);

        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, m_vertexAttributeLists.bitangent,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, m_vertexAttributeBuffers.bitangentBuffer, m_vertexAttributeBuffers.bitangentBufferMemory);

        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, m_vertexAttributeLists.matIdx,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, m_vertexAttributeBuffers.matIdxBuffer, m_vertexAttributeBuffers.matIdxBufferMemory);

        //VK_FORMAT_R32G32_SFLOAT
        createBufferView(m_context->m_device, m_vertexAttributeBuffers.positionBuffer, VK_FORMAT_R32G32B32_SFLOAT, m_vertexAttributeBuffers.positionView, m_vertexAttributeLists.pos);
        createBufferView(m_context->m_device, m_vertexAttributeBuffers.normalBuffer, VK_FORMAT_R32G32B32_SFLOAT, m_vertexAttributeBuffers.normalView, m_vertexAttributeLists.normal);
        createBufferView(m_context->m_device, m_vertexAttributeBuffers.uvCoordBuffer, VK_FORMAT_R32G32_SFLOAT, m_vertexAttributeBuffers.uvCoordView, m_vertexAttributeLists.uvCoord);
        createBufferView(m_context->m_device, m_vertexAttributeBuffers.tangentBuffer, VK_FORMAT_R32G32B32_SFLOAT, m_vertexAttributeBuffers.tangentView, m_vertexAttributeLists.tangent);
        createBufferView(m_context->m_device, m_vertexAttributeBuffers.bitangentBuffer, VK_FORMAT_R32G32B32_SFLOAT, m_vertexAttributeBuffers.bitangentView, m_vertexAttributeLists.bitangent);
        createBufferView(m_context->m_device, m_vertexAttributeBuffers.matIdxBuffer, VK_FORMAT_R32_UINT, m_vertexAttributeBuffers.matIdxView, m_vertexAttributeLists.matIdx);

        m_loadStatus |= VERTEX_ATTRIBUTE_BUFFER_LOADED;
    }
}

void  Scene::createIndexBuffer() {
    if (!(m_loadStatus & INDEX_BUFFER_LOADED))
    {
        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, *m_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_indexBuffer, m_indexBufferDeviceMemory);
        m_loadStatus |= INDEX_BUFFER_LOADED;
    }
}

void  Scene::createDecalVertexBuffer() {
    if (!(m_loadStatus & DECAL_VERTEX_BUFFER_LOADED))
    {
        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, m_decal_vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_decal_vertexBuffer, m_decal_vertexBufferDeviceMemory);
        m_loadStatus |= DECAL_VERTEX_BUFFER_LOADED;
    }
}

void  Scene::createDecalIndexBuffer() {
    if (!(m_loadStatus & DECAL_INDEX_BUFFER_LOADED))
    {
        createAndUploadBuffer(m_context->m_device, m_context->m_physicalDevice, m_context->m_queue, m_context->m_commandPool, m_decal_indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, m_decal_indexBuffer, m_decal_indexBufferDeviceMemory);
        m_loadStatus |= DECAL_INDEX_BUFFER_LOADED;
    }
}

void  Scene::recreateDecalAccelerationStructure()
{
    if ((m_loadStatus & ACCELERATION_STRUCTURE_LOADED))
    {
        m_asBuilder->destroyAccelerationStructure(m_context->m_device, m_context->m_instance, &m_decal_as);
    }
    m_decal_as = m_asBuilder->createDecalAccelerationStructure(m_context->m_device, m_context->m_instance, m_context->m_physicalDevice, m_context->m_commandPool, m_context->m_queue, m_rootNode);
    m_loadStatus |= ACCELERATION_STRUCTURE_LOADED;

}
void  Scene::recreateLightAccelerationStructure()
{
    if ((m_loadStatus & ACCELERATION_STRUCTURE_LOADED))
    {
        m_asBuilder->destroyAccelerationStructure(m_context->m_device, m_context->m_instance, &m_point_light_as);
    }
    m_point_light_as = m_asBuilder->createPointLightAccelerationStructure(m_context->m_device, m_context->m_instance, m_context->m_physicalDevice, m_context->m_commandPool, m_context->m_queue, m_rootNode);
    m_loadStatus |= ACCELERATION_STRUCTURE_LOADED;

}

void  Scene::createAccelerationStructure()
{
    if (!(m_loadStatus & ACCELERATION_STRUCTURE_LOADED))
    {
        m_decal_as = m_asBuilder->createDecalAccelerationStructure(m_context->m_device, m_context->m_instance, m_context->m_physicalDevice, m_context->m_commandPool, m_context->m_queue, m_rootNode);
        if (!DISABLE_SCENE_AS)m_scene_as = m_asBuilder->createMeshAccelerationStructure(m_context->m_device, m_context->m_instance, m_context->m_physicalDevice, m_context->m_commandPool, m_context->m_queue, m_rootNode);
        m_point_light_as = m_asBuilder->createPointLightAccelerationStructure(m_context->m_device, m_context->m_instance, m_context->m_physicalDevice, m_context->m_commandPool, m_context->m_queue, m_rootNode);
        m_loadStatus |= ACCELERATION_STRUCTURE_LOADED;
    }

}

void  Scene::destroy()
{
    if (m_loadStatus & ACCELERATION_STRUCTURE_LOADED)
    {
        m_asBuilder->destroyAccelerationStructure(m_context->m_device, m_context->m_instance, &m_decal_as);
        m_asBuilder->destroyAccelerationStructure(m_context->m_device, m_context->m_instance, &m_scene_as);
        m_asBuilder->destroyAccelerationStructure(m_context->m_device, m_context->m_instance, &m_point_light_as);
    }
    if (m_loadStatus & INDEX_BUFFER_LOADED)
    {
        vkFreeMemory(m_context->m_device, m_indexBufferDeviceMemory, nullptr);
        vkDestroyBuffer(m_context->m_device, m_indexBuffer, nullptr);
    }
    if (m_loadStatus & VERTEX_BUFFER_LOADED)
    {
        vkFreeMemory(m_context->m_device, m_vertexBufferDeviceMemory, nullptr);
        vkDestroyBuffer(m_context->m_device, m_vertexBuffer, nullptr);
    }
    if (m_loadStatus & DECAL_INDEX_BUFFER_LOADED)
    {
        vkFreeMemory(m_context->m_device, m_decal_indexBufferDeviceMemory, nullptr);
        vkDestroyBuffer(m_context->m_device, m_decal_indexBuffer, nullptr);
    }
    if (m_loadStatus & DECAL_VERTEX_BUFFER_LOADED)
    {
        vkFreeMemory(m_context->m_device, m_decal_vertexBufferDeviceMemory, nullptr);
        vkDestroyBuffer(m_context->m_device, m_decal_vertexBuffer, nullptr);
    }
    if (m_loadStatus & VERTEX_ATTRIBUTE_BUFFER_LOADED)
    {
        for (size_t i = 0; i < VERTEX_ATTRIBUTE_COUNT; i++)
        {
            vkDestroyBufferView(m_context->m_device, m_vertexAttributeBuffers.views[i], nullptr);
            vkDestroyBuffer(m_context->m_device, m_vertexAttributeBuffers.buffers[i], nullptr);
            vkFreeMemory(m_context->m_device, m_vertexAttributeBuffers.memory[i], nullptr);
        }
    }
    if (m_loadStatus & MATERIALS_LOADED)
    {
        for (uint32_t i = 0; i < m_images.size(); i++) {
            m_images[i]->destroy();
        }
    }
    m_loadStatus = 0x00000000;

}

Scene::~Scene()
{
}

