#pragma once

#include "Vertex.h"
#include "VulkanUtils.h"

struct Decal
{
	glm::mat4 inverseTransposedTransformation;
	glm::mat4 inverseTransformation;
	glm::uint materialIndex;
	VkAabbPositionsKHR aabb;
	glm::vec2 uvCoord[4];
	float weight;
	uint32_t channel;
	// Only For Deferred Decals
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};