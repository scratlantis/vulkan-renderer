#pragma once
#include "VulkanUtils.h"

struct PointLight
{
	VkAabbPositionsKHR aabb;
	glm::vec4 color;
	glm::vec3 position;
};