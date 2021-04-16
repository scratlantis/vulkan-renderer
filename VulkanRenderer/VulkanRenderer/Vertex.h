#pragma once

#include <glm/glm.hpp>
#include "VulkanUtils.h"
#include <glm/gtx/hash.hpp>

class Vertex {
public:
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 uvCoord;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::uint matIdx;


    Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 uvCoord, glm::vec3 normal, glm::uint matIdx, glm::vec3 tangent, glm::vec3 bitangent)
        :pos(pos), color(color), uvCoord(uvCoord), normal(normal), matIdx(matIdx), tangent(tangent), bitangent(bitangent)
    {}

    void setTangentAndBitangent(glm::vec3 tangent, glm::vec3 bitangent)
    {
        this->tangent = tangent;
        this->bitangent = bitangent;
    }

    bool operator==(const Vertex& other) const
    {
        //TODO update
        return pos == other.pos && color == other.color && uvCoord == other.uvCoord && normal == other.normal;
    }



    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription vertexInputBindingDescription;
        vertexInputBindingDescription.binding = 0;
        vertexInputBindingDescription.stride = sizeof(Vertex);
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return vertexInputBindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions(7);


        vertexInputAttributeDescriptions[0].location = 0;
        vertexInputAttributeDescriptions[0].binding = 0;
        vertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputAttributeDescriptions[0].offset = offsetof(Vertex, pos);

        vertexInputAttributeDescriptions[1].location = 1;
        vertexInputAttributeDescriptions[1].binding = 0;
        vertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputAttributeDescriptions[1].offset = offsetof(Vertex, color);

        vertexInputAttributeDescriptions[2].location = 2;
        vertexInputAttributeDescriptions[2].binding = 0;
        vertexInputAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        vertexInputAttributeDescriptions[2].offset = offsetof(Vertex, uvCoord);

        vertexInputAttributeDescriptions[3].location = 3;
        vertexInputAttributeDescriptions[3].binding = 0;
        vertexInputAttributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputAttributeDescriptions[3].offset = offsetof(Vertex, normal);

        vertexInputAttributeDescriptions[4].location = 4;
        vertexInputAttributeDescriptions[4].binding = 0;
        vertexInputAttributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputAttributeDescriptions[4].offset = offsetof(Vertex, tangent);


        vertexInputAttributeDescriptions[5].location = 5;
        vertexInputAttributeDescriptions[5].binding = 0;
        vertexInputAttributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputAttributeDescriptions[5].offset = offsetof(Vertex, bitangent);


        vertexInputAttributeDescriptions[6].location = 6;
        vertexInputAttributeDescriptions[6].binding = 0;
        vertexInputAttributeDescriptions[6].format = VK_FORMAT_R32_UINT;
        vertexInputAttributeDescriptions[6].offset = offsetof(Vertex, matIdx);

        return vertexInputAttributeDescriptions;
    }

};


namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vert) const
        {
            size_t h1 = hash<glm::vec3>()(vert.pos);
            size_t h2 = hash<glm::vec3>()(vert.color);
            size_t h3 = hash<glm::vec2>()(vert.uvCoord);
            size_t h4 = hash<glm::vec3>()(vert.normal);


            // TODO update Hash
            return ((((h1 ^ (h2 << 1)) >> 1) ^ h3) << 1) ^ h4;
        }
    };
}