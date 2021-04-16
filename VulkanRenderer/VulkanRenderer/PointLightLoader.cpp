#pragma once
#include <glm/glm.hpp>
#include "PointLight.h"
#include "PointLightLoader.h"
#include "VulkanUtils.h"
#include <iostream>
#include <stdlib.h>


#define EPSIOLON 0.01

#define MIN_LIGHT_THRESHOLD (5.0/256.0)

using namespace VulkanUtils;

float PointLightLoader::getLightSphereRadious(float intensity)
{
	float r = sqrt(intensity / MIN_LIGHT_THRESHOLD); // Should work with our lights
	return r;
}

void PointLightLoader::loadPointLight(tinyobj::shape_t shape, tinyobj::attrib_t* vertexAttributes, Node& node)
{
	const uint32_t indexCount = 3 * 2 * 6;
	tinyobj::index_t indices[indexCount];
	for (size_t i = 0; i < indexCount; i++)
	{
		indices[i] = shape.mesh.indices[i];
	}


	glm::vec3 position = glm::vec3(0.0);
	for (size_t i = 0; i < indexCount; i++)
	{
		position += glm::vec3(
			-vertexAttributes->vertices[3 * indices[i].vertex_index + 0],
			vertexAttributes->vertices[3 * indices[i].vertex_index + 2],  //Swap y and z axes, because z is up, not y
			vertexAttributes->vertices[3 * indices[i].vertex_index + 1]
		);
	}
	position /= indexCount;

	PointLight pointLight = {};
	pointLight.position = position;

	// Parse object name.

	char delimiter = '.';
	std::string lightName = shape.name;
	std::vector<std::string> tokens = split(lightName, delimiter);
	if (tokens.size() < 4)
	{
		std::cout << "Light: " << shape.name.c_str() << " cannot be parsed." << std::endl;
		return;
	}
	pointLight.color = glm::vec4(0.0);
	pointLight.color.r = stringToFloat(tokens[0]);
	pointLight.color.g = stringToFloat(tokens[1]);
	pointLight.color.b = stringToFloat(tokens[2]);
	pointLight.color.a = stringToFloat(tokens[3]);

	pointLight.color.a *= 10.0; // test
	pointLight.color.r = 252.0 / 255.0;
	pointLight.color.g = 156.0 / 255.0;
	pointLight.color.b = 84.0 / 255.0;

	//pointLight.color = glm::vec4(1.0);
	float lightRadius = getLightSphereRadious(pointLight.color.a);



	//Calculate AABB
	pointLight.aabb.minX = pointLight.position.x - lightRadius;
	pointLight.aabb.minY = pointLight.position.y - lightRadius;
	pointLight.aabb.minZ = pointLight.position.z - lightRadius;
	pointLight.aabb.maxX = pointLight.position.x + lightRadius;
	pointLight.aabb.maxY = pointLight.position.y + lightRadius;
	pointLight.aabb.maxZ = pointLight.position.z + lightRadius;

	node.m_pointLights.push_back(pointLight);
}

void PointLightLoader::generatePointLight(glm::vec3 position, Node& node)
{

	PointLight pointLight = {};
	pointLight.position = position;

	// Parse object name.
	//pointLight.color.a = 0.5; // test
	pointLight.color.a = 0.05; // test
	//pointLight.color.a = 0.005; // test

	pointLight.color.r = (rand() % 255) / 255.0;
	pointLight.color.g = (rand() % 255) / 255.0;
	pointLight.color.b = (rand() % 255) / 255.0;

	//pointLight.color = glm::vec4(1.0);
	float lightRadius = getLightSphereRadious(pointLight.color.a);



	//Calculate AABB
	pointLight.aabb.minX = pointLight.position.x - lightRadius;
	pointLight.aabb.minY = pointLight.position.y - lightRadius;
	pointLight.aabb.minZ = pointLight.position.z - lightRadius;
	pointLight.aabb.maxX = pointLight.position.x + lightRadius;
	pointLight.aabb.maxY = pointLight.position.y + lightRadius;
	pointLight.aabb.maxZ = pointLight.position.z + lightRadius;

	node.m_pointLights.push_back(pointLight);
}
void PointLightLoader::generatePointLight(glm::vec3 position, glm::vec4 color, Node& node)
{

	PointLight pointLight = {};
	pointLight.position = position;

	// Parse object name.
	pointLight.color.a = color.a;
	pointLight.color.r = color.r;
	pointLight.color.g = color.g;
	pointLight.color.b = color.b;

	//pointLight.color = glm::vec4(1.0);
	float lightRadius = getLightSphereRadious(pointLight.color.a);



	//Calculate AABB
	pointLight.aabb.minX = pointLight.position.x - lightRadius;
	pointLight.aabb.minY = pointLight.position.y - lightRadius;
	pointLight.aabb.minZ = pointLight.position.z - lightRadius;
	pointLight.aabb.maxX = pointLight.position.x + lightRadius;
	pointLight.aabb.maxY = pointLight.position.y + lightRadius;
	pointLight.aabb.maxZ = pointLight.position.z + lightRadius;

	node.m_pointLights.push_back(pointLight);
}

PointLightLoader::PointLightLoader()
{
}

PointLightLoader::~PointLightLoader()
{
}

