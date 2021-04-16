#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include "Vertex.h"
#include "MeshLoader.h"
#include <string>
#include "Mesh.h"
#include "Node.h"


MeshLoader::MeshLoader(MaterialLoader* materialLoader, DecalLoader* decalLoader, PointLightLoader* pointLightLoader) : m_materialLoader(materialLoader), m_decalLoader(decalLoader), m_pointLightLoader(pointLightLoader)
{

}

MeshLoader::MeshLoader()
{

}

void MeshLoader::load_Decals(const char* textureBaseDir) {
	m_materialLoader->load_Decals(textureBaseDir);
}

void MeshLoader::createMesh(const char* path, const char* mtlBaseDir, const char* textureBaseDir, Node& node)
{
	tinyobj::attrib_t vertexAttributes;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string errorString;
	std::string warningString;

	Mesh mesh;

	int lightCounter = 0;
	int decalCounter = 0;

	bool success = tinyobj::LoadObj(&vertexAttributes, &shapes, &materials, &warningString, &errorString, path, mtlBaseDir);

	if (!success)
	{
		std::cout << "TINYOBJ WARNING:" << warningString << std::endl;
		throw std::runtime_error(errorString);

	}
	std::cout << "Material Count: " << materials.size() << std::endl;

	if (m_materialLoader == nullptr) {
		throw std::runtime_error("Material Loader not set");
	}

	m_materialLoader->load(materials, textureBaseDir);
	load_Decals("models/DecalTextures");
	std::vector<int> decalMaterialIndices = m_materialLoader->getDecalMaterialIndices();

	std::unordered_map<Vertex, uint32_t> vertices;



	for (size_t s = 0; s < shapes.size(); s++)
	{
		if (shapes[s].name._Starts_with("DECAL"))
		{
			std::cout << "found decal!!" << std::endl;
			m_decalLoader->loadDecal(shapes[s], &vertexAttributes, node);


		}
		else if (shapes[s].name._Starts_with("POINT_LIGHT"))
		{
			std::cout << "found point light!!" << std::endl;
			m_pointLightLoader->loadPointLight(shapes[s], &vertexAttributes, node);
		}
		else
		{




			std::cout << "loading shape " << shapes[s].name << " at index: " << s << " from " << shapes.size() << std::endl;
			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				uint32_t fv = shapes[s].mesh.num_face_vertices[f];


				for (size_t v = 0; v < fv; v++)
				{
					tinyobj::index_t index = shapes[s].mesh.indices[index_offset + v];

					glm::vec3 pos = {
						-vertexAttributes.vertices[3 * index.vertex_index + 0],
						vertexAttributes.vertices[3 * index.vertex_index + 2],  //Swap y and z axes, because z is up, not y
						vertexAttributes.vertices[3 * index.vertex_index + 1]
					};

					glm::vec3 normal = {
						-vertexAttributes.normals[3 * index.normal_index + 0],
						vertexAttributes.normals[3 * index.normal_index + 2],//Swap y and z axes, because z is up, not y
						vertexAttributes.normals[3 * index.normal_index + 1]
					};



					glm::vec2 uv;
					if (2 * index.texcoord_index + 1 < vertexAttributes.texcoords.size())
					{
						uv = {
							vertexAttributes.texcoords[2 * index.texcoord_index + 0],
							vertexAttributes.texcoords[2 * index.texcoord_index + 1]
						};
					}
					else
					{
						uv = glm::vec2(0, 0);
					}
					int matIdx = shapes[s].mesh.material_ids[f];
					// Adds A LOT of decals
					if (GENERATE_DECALS && f % 1000 == 0 && m_materialLoader->getMaterialProperties()[matIdx].idx_map_alpha == 0xFFFFFFFF)// && dot(normal, glm::vec3(0.0,0.0,-1.0))>0.9)
					{
						int material = decalMaterialIndices[rand() % decalMaterialIndices.size()];
						m_decalLoader->generateDecal(pos, normal, 1.0, material, node);

						decalCounter++;
					}
					// Adds A LOT of light sources
					if (GENERATE_LIGHTS && f % 1000 == 0 && m_materialLoader->getMaterialProperties()[matIdx].idx_map_alpha == 0xFFFFFFFF)
					{
						m_pointLightLoader->generatePointLight(pos + glm::vec3(node.m_transformMatrix * glm::vec4(10.0f * normal, 0.0)), node);
						lightCounter++;
					}




					Vertex vert(pos, glm::normalize(pos), uv, normal, matIdx, glm::vec3(0.0), glm::vec3(0.0));


					if (vertices.count(vert) == 0)
					{
						vertices[vert] = vertices.size();
						mesh.m_vertices.push_back(vert);
					}

					mesh.m_indices.push_back(vertices[vert]);
				}
				index_offset += fv;

			};
		}

	}

	std::cout << "Done loading!" << std::endl;
	std::cout << "Light Count: " << lightCounter << std::endl;
	std::cout << "Decal Count: " << decalCounter << std::endl;

	//std::reverse(std::begin(m_indices), std::end(m_indices));

	for (size_t i = 0; i < mesh.m_indices.size() - 2; i += 3)
	{

		// Shortcuts for vertices
		glm::vec3 v0 = mesh.m_vertices[mesh.m_indices[i + 0]].pos;
		glm::vec3 v1 = mesh.m_vertices[mesh.m_indices[i + 1]].pos;
		glm::vec3 v2 = mesh.m_vertices[mesh.m_indices[i + 2]].pos;

		// Shortcuts for UVs
		glm::vec2 uv0 = mesh.m_vertices[mesh.m_indices[i + 0]].uvCoord;
		glm::vec2 uv1 = mesh.m_vertices[mesh.m_indices[i + 1]].uvCoord;
		glm::vec2 uv2 = mesh.m_vertices[mesh.m_indices[i + 2]].uvCoord;

		// Edges of the triangle : position delta
		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;

		//uv0.y = 1.0 - uv0.y;
		//uv1.y = 1.0 - uv1.y;
		//uv2.y = 1.0 - uv2.y;

		// UV delta
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;


		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

		mesh.m_vertices[mesh.m_indices[i + 0]].tangent = tangent;
		mesh.m_vertices[mesh.m_indices[i + 1]].tangent = tangent;
		mesh.m_vertices[mesh.m_indices[i + 2]].tangent = tangent;

		mesh.m_vertices[mesh.m_indices[i + 0]].bitangent += bitangent;
		mesh.m_vertices[mesh.m_indices[i + 1]].bitangent += bitangent;
		mesh.m_vertices[mesh.m_indices[i + 2]].bitangent += bitangent;

	}

	for (Vertex vertex : mesh.m_vertices)
	{
		vertex.tangent = glm::normalize(vertex.tangent);
		vertex.bitangent = glm::normalize(vertex.bitangent);
	}


	node.m_meshs.push_back(mesh);

}

void MeshLoader::setMaterialLoader(MaterialLoader* materialLoader)
{
	this->m_materialLoader = materialLoader;
}

void MeshLoader::setDecalLoader(DecalLoader* decalLoader)
{
	this->m_decalLoader = decalLoader;
}

void MeshLoader::setPointLightLoader(PointLightLoader* pointLightLoader)
{
	this->m_pointLightLoader = pointLightLoader;
}