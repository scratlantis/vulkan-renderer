#pragma once

#include <glm/glm.hpp>
#include "Decal.h"
#include "Node.h"
#include "Vertex.h"
#include "VulkanUtils.h"
#include "DecalLoader.h"
#include <iostream>


#define EPSIOLON 0.01

using namespace VulkanUtils;





void DecalLoader::loadDecal(tinyobj::shape_t shape, tinyobj::attrib_t* vertexAttributes, Node& node)
{
	const uint32_t indexCount = 3 * 2 * 6;
	tinyobj::index_t indices[indexCount];
	for (size_t i = 0; i < indexCount; i++)
	{
		indices[i] = shape.mesh.indices[i];
	}

	glm::vec3 positions[3]; //3 out of 4 vertex positions are sufficient
	glm::vec2 uvCoord[4];
	//Get position and uv coord of vertices of the projected faces (always 2 triangles because its a quad)
	for (size_t i = 0; i < 3; i++)
	{
		positions[i] = {
							-vertexAttributes->vertices[3 * indices[i].vertex_index + 0],
							vertexAttributes->vertices[3 * indices[i].vertex_index + 2],  //Swap y and z axes, because z is up, not y
							vertexAttributes->vertices[3 * indices[i].vertex_index + 1]
		};
		uvCoord[i] = {
							vertexAttributes->texcoords[2 * indices[i].texcoord_index + 0],
							vertexAttributes->texcoords[2 * indices[i].texcoord_index + 1]
		};

	}

	//Get last missing uv coord
	for (size_t i = 3; i < 6; i++)
	{
		glm::vec2 uv = {
							vertexAttributes->texcoords[2 * indices[i].texcoord_index + 0],
							vertexAttributes->texcoords[2 * indices[i].texcoord_index + 1]
		};
		if (uv != uvCoord[0] && uv != uvCoord[1] && uv != uvCoord[2]) {
			uvCoord[3] = uv;
			break;
		}

	}

	glm::vec3 xAxis;
	glm::vec3 yAxis;

	// Calculate permutation
	int permutation;
	for (size_t i = 0; i < 2; i++)
	{
		xAxis = glm::normalize(positions[(1 + i) % 3] - positions[(0 + i) % 3]);
		yAxis = glm::normalize(positions[(2 + i) % 3] - positions[(1 + i) % 3]);

		if (glm::abs(glm::dot(xAxis, yAxis)) < EPSIOLON) {
			permutation = i;
			break;
		}

	}

	Decal decal;

	glm::vec3 origin = positions[permutation];
	glm::vec3 originPlusX = positions[(1 + permutation) % 3];
	glm::vec3 originPlusXY = positions[(2 + permutation) % 3];
	decal.uvCoord[0] = uvCoord[permutation];
	decal.uvCoord[1] = uvCoord[(permutation + 1) % 3];
	decal.uvCoord[2] = uvCoord[(permutation + 2) % 3];
	decal.uvCoord[3] = uvCoord[3];


	glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, yAxis)); // Sign stil to be determined


	float backFaceDistance;
	//Get one more vertex to determin Sign of zAxis
	for (size_t i = 6; i < 9; i++)
	{
		glm::vec3 pos = {
							-vertexAttributes->vertices[3 * indices[i].vertex_index + 0],
							vertexAttributes->vertices[3 * indices[i].vertex_index + 2],  //Swap y and z axes, because z is up, not y
							vertexAttributes->vertices[3 * indices[i].vertex_index + 1]
		};

		if (glm::dot(normalize(pos - origin), zAxis) > EPSIOLON)
		{
			backFaceDistance = glm::dot(pos - origin, zAxis);
			break;
		}
		else if (glm::dot(normalize(pos - origin), zAxis) < -EPSIOLON)
		{
			zAxis *= -1;
			backFaceDistance = glm::dot(pos - origin, zAxis);
			break;
		}


	}

	glm::vec3 dimensions = glm::vec3(glm::distance(origin, originPlusX), glm::distance(originPlusX, originPlusXY), backFaceDistance);
	glm::mat4 transformation = glm::mat4(
		glm::vec4(xAxis * dimensions.x, 0),
		glm::vec4(yAxis * dimensions.y, 0),
		glm::vec4(zAxis * dimensions.z, 0),
		glm::vec4(origin, 1));
	decal.inverseTransformation = glm::inverse(transformation);
	decal.inverseTransposedTransformation = glm::transpose(decal.inverseTransformation);
	decal.materialIndex = shape.mesh.material_ids[0];

	char delimiter = '.';
	std::string decalName = shape.name;
	std::vector<std::string> tokens = split(decalName, delimiter);
	if (tokens.size() < 3)
	{
		std::cout << "Decal: " << shape.name.c_str() << " cannot be parsed." << std::endl;
		return;
	}
	decal.weight = stringToFloat(tokens[1]);
	decal.channel = stringToInt(tokens[2]);




	//Prepare AABB data
	VkAabbPositionsKHR aabb;
	aabb.minX = std::numeric_limits<float>::max();
	aabb.minY = std::numeric_limits<float>::max();
	aabb.minZ = std::numeric_limits<float>::max();

	aabb.maxX = std::numeric_limits<float>::lowest();
	aabb.maxY = std::numeric_limits<float>::lowest();
	aabb.maxZ = std::numeric_limits<float>::lowest();


	for (size_t i = 0; i < indexCount; i++)
	{
		glm::vec3 pos = {
							-vertexAttributes->vertices[3 * indices[i].vertex_index + 0],
							vertexAttributes->vertices[3 * indices[i].vertex_index + 2],  //Swap y and z axes, because z is up, not y
							vertexAttributes->vertices[3 * indices[i].vertex_index + 1]
		};

		Vertex vert(pos, glm::normalize(pos), glm::vec2(0.0), glm::vec3(0.0), 0, glm::vec3(0.0), glm::vec3(0.0));

		decal.vertices.push_back(vert);
		decal.indices.push_back(decal.indices.size());

		aabb.minX = glm::min(aabb.minX, pos.x);
		aabb.minY = glm::min(aabb.minY, pos.y);
		aabb.minZ = glm::min(aabb.minZ, pos.z);

		aabb.maxX = glm::max(aabb.maxX, pos.x);
		aabb.maxY = glm::max(aabb.maxY, pos.y);
		aabb.maxZ = glm::max(aabb.maxZ, pos.z);

	}

	decal.aabb = aabb;

	std::cout << "Decal mat: " << decal.materialIndex << std::endl;
	node.m_decals.push_back(decal);

}




void DecalLoader::generateDecal(glm::mat4 transformation, uint32_t materialIndex, Node& node, uint32_t weight, uint32_t channel)
{
	Decal decal;
	decal.uvCoord[0] = glm::vec2(0.0, 0.0);
	decal.uvCoord[1] = glm::vec2(1.0, 0.0);
	decal.uvCoord[2] = glm::vec2(1.0, 1.0);
	decal.uvCoord[3] = glm::vec2(0.0, 1.0);
	decal.inverseTransformation = glm::inverse(transformation);
	decal.inverseTransposedTransformation = glm::transpose(decal.inverseTransformation);
	decal.materialIndex = materialIndex;
	decal.weight = weight;
	decal.channel = channel;

	//Prepare AABB data
	VkAabbPositionsKHR aabb;
	aabb.minX = std::numeric_limits<float>::max();
	aabb.minY = std::numeric_limits<float>::max();
	aabb.minZ = std::numeric_limits<float>::max();

	aabb.maxX = std::numeric_limits<float>::lowest();
	aabb.maxY = std::numeric_limits<float>::lowest();
	aabb.maxZ = std::numeric_limits<float>::lowest();


	std::vector<glm::vec3> cubeVertecies =
	{
		glm::vec3(-1.0, -1.0,  1.0),
		glm::vec3(1.0, -1.0,  1.0),
		glm::vec3(1.0,  1.0,  1.0),
		glm::vec3(-1.0,  1.0,  1.0),
		glm::vec3(-1.0, -1.0, -1.0),
		glm::vec3(1.0, -1.0, -1.0),
		glm::vec3(1.0,  1.0, -1.0),
		glm::vec3(-1.0,  1.0, -1.0)
	};
	std::vector<uint32_t> cubeIndices =
	{
		// front
		0, 1, 2,
		2, 3, 0,
		// right
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// left
		4, 0, 3,
		3, 7, 4,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// top
		3, 2, 6,
		6, 7, 3
	};
	for (size_t i = 0; i < 36; i++)
	{
		glm::vec3 pos = 0.5f * (cubeVertecies[cubeIndices[i]] + glm::vec3(1.0, 1.0, 1.0));
		pos = glm::vec3(transformation * glm::vec4(pos, 1.0));
		Vertex vert(pos, glm::normalize(pos), glm::vec2(0.0), glm::vec3(0.0), 0, glm::vec3(0.0), glm::vec3(0.0));
		decal.vertices.push_back(vert);
		decal.indices.push_back(i);

		aabb.minX = glm::min(aabb.minX, pos.x);
		aabb.minY = glm::min(aabb.minY, pos.y);
		aabb.minZ = glm::min(aabb.minZ, pos.z);

		aabb.maxX = glm::max(aabb.maxX, pos.x);
		aabb.maxY = glm::max(aabb.maxY, pos.y);
		aabb.maxZ = glm::max(aabb.maxZ, pos.z);

	}
	decal.aabb = aabb;
	node.m_decals.push_back(decal);
}






void DecalLoader::generateDecal(glm::vec3 position, glm::vec3 normal, float scale, uint32_t materialIndex, Node& node)
{
	glm::vec3 dimensions = glm::vec3(scale);
	glm::vec3 zAxis = normalize(normal);
	glm::vec3 xAxis;
	if (abs(dot(zAxis, glm::vec3(1.0, 0.0, 0.0))) < 0.9)
	{
		xAxis = cross(zAxis, glm::vec3(1.0, 0.0, 0.0));
	}
	else
	{
		xAxis = cross(zAxis, glm::vec3(0.0, 1.0, 0.0));
	}
	glm::vec3 yAxis = -cross(xAxis, zAxis);

	glm::mat4 transformation = glm::mat4(
		glm::vec4(xAxis * dimensions.x, 0),
		glm::vec4(yAxis * dimensions.y, 0),
		glm::vec4(zAxis * dimensions.z, 0),
		//glm::vec4(glm::vec3(position.x, position.y, 0.5f*dimensions.z), 1));
		glm::vec4(glm::vec3(position.x, position.y, position.z), 1));

	generateDecal(transformation, materialIndex, node, 1, 0);
}



DecalLoader::DecalLoader()
{
}

DecalLoader::~DecalLoader()
{
}



