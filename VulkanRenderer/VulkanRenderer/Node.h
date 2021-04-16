#pragma once
#include "VulkanUtils.h"
#include "Mesh.h"
#include "Decal.h"
#include "PointLight.h"

struct Node
{
	std::vector<Node*> m_childNodes;
	glm::mat4 m_transformMatrix;
	std::vector<Decal> m_decals;
	std::vector<Mesh> m_meshs;
	std::vector<PointLight> m_pointLights;
};