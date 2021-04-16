#pragma once




#include "VulkanUtils.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <iostream>
#include "MaterialLoader.h"

using namespace VulkanUtils;



MaterialLoader::MaterialLoader()
{
	m_textures_map[""] = 0;
}

void MaterialLoader::load(std::vector<tinyobj::material_t> materials, std::string baseDir) {
	for (size_t m = 0; m < materials.size(); m++)
	{
		MaterialProperties mp;

		mp.ambient = glm::f32vec3(materials[m].ambient[0], materials[m].ambient[1], materials[m].ambient[2]);
		mp.diffuse = glm::f32vec3(materials[m].diffuse[0], materials[m].diffuse[1], materials[m].diffuse[2]);
		mp.specular = glm::f32vec3(materials[m].specular[0], materials[m].specular[1], materials[m].specular[2]);
		mp.transparency = materials[m].dissolve;

		if (m_textures_map.count(materials[m].specular_texname) == 0)
		{
			m_textures_map[materials[m].specular_texname] = m_textures_map.size();
			m_textures_list.push_back(baseDir + "/" + materials[m].specular_texname);
			m_useSRGB.push_back(true);

		}
		if (m_textures_map.count(materials[m].diffuse_texname) == 0)
		{
			m_textures_map[materials[m].diffuse_texname] = m_textures_map.size();
			m_textures_list.push_back(baseDir + "/" + materials[m].diffuse_texname);
			m_useSRGB.push_back(true);
		}
		if (m_textures_map.count(materials[m].bump_texname) == 0)
		{
			m_textures_map[materials[m].bump_texname] = m_textures_map.size();
			m_textures_list.push_back(baseDir + "/" + materials[m].bump_texname);
			m_useSRGB.push_back(false);
		}
		if (m_textures_map.count(materials[m].alpha_texname) == 0)
		{
			m_textures_map[materials[m].alpha_texname] = m_textures_map.size();
			m_textures_list.push_back(baseDir + "/" + materials[m].alpha_texname);
			m_useSRGB.push_back(true);
		}
		if (m_textures_map.count(materials[m].emissive_texname) == 0)
		{
			m_textures_map[materials[m].emissive_texname] = m_textures_map.size();
			m_textures_list.push_back(baseDir + "/" + materials[m].emissive_texname);
			m_useSRGB.push_back(true);
		}

		mp.idx_map_specular = m_textures_map[materials[m].specular_texname] - 1;
		mp.idx_map_diffuse = m_textures_map[materials[m].diffuse_texname] - 1;
		mp.idx_map_normal = m_textures_map[materials[m].bump_texname] - 1;
		mp.idx_map_alpha = m_textures_map[materials[m].alpha_texname] - 1;
		mp.idx_map_emissive = m_textures_map[materials[m].emissive_texname] - 1;

		m_materialProperties.push_back(mp);

	}

}

void MaterialLoader::load_Decals(std::string baseDir)
{
	std::vector<std::string> texture_folders;
	for (auto& p : std::filesystem::recursive_directory_iterator(baseDir))
	{
		if (p.is_directory())
		{
			texture_folders.push_back(p.path().string());
		}
	}

	for (size_t i = 0; i < texture_folders.size(); i++)
	{
		MaterialProperties mp;
		mp.idx_map_specular = UINT32_MAX;
		mp.idx_map_diffuse = UINT32_MAX;
		mp.idx_map_normal = UINT32_MAX;
		mp.idx_map_alpha = UINT32_MAX;
		mp.idx_map_emissive = UINT32_MAX;
		for (const auto& entry : std::filesystem::directory_iterator(texture_folders[i]))
		{
			std::cout << entry.path() << std::endl;
			std::string suffix = split(entry.path().string(), '.')[0];
			auto string_vector = split(suffix, '_');
			suffix = string_vector[string_vector.size() - 1];
			std::string texPath = entry.path().string();

			if (suffix == "specular")
			{
				m_textures_map[texPath] = m_textures_map.size();
				m_textures_list.push_back(texPath);
				m_useSRGB.push_back(true);
				mp.idx_map_specular = m_textures_map[texPath] - 1;
			}
			if (suffix == "albedo")
			{
				m_textures_map[texPath] = m_textures_map.size();
				m_textures_list.push_back(texPath);
				m_useSRGB.push_back(true);
				mp.idx_map_diffuse = m_textures_map[texPath] - 1;
			}
			if (suffix == "normal")
			{
				m_textures_map[texPath] = m_textures_map.size();
				m_textures_list.push_back(texPath);
				m_useSRGB.push_back(false);
				mp.idx_map_normal = m_textures_map[texPath] - 1;
			}
			if (suffix == "alpha")
			{
				m_textures_map[texPath] = m_textures_map.size();
				m_textures_list.push_back(texPath);
				m_useSRGB.push_back(true);
				mp.idx_map_alpha = m_textures_map[texPath] - 1;
			}
			if (suffix == "emissive")
			{
				m_textures_map[texPath] = m_textures_map.size();
				m_textures_list.push_back(texPath);
				m_useSRGB.push_back(true);
				mp.idx_map_emissive = m_textures_map[texPath] - 1;
			}
		}
		m_materialProperties.push_back(mp);
		m_decalMaterialIndices.push_back(m_materialProperties.size() - 1);
	}
}


void MaterialLoader::uploadMaterials(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
	std::vector<TextureImage*>* images)
{


	TextureImage::loadImages(device, physicalDevice, commandPool, queue, images, m_textures_list, m_useSRGB);

	m_textures_list.clear();

}

std::vector<MaterialProperties> MaterialLoader::getMaterialProperties()
{
	return m_materialProperties;
}

std::vector<int> MaterialLoader::getDecalMaterialIndices()
{
	return m_decalMaterialIndices;
}

size_t MaterialLoader::getTextureCount()
{
	return m_textures_map.size() - 1;
}

size_t MaterialLoader::getMaterialCount()
{
	return m_materialProperties.size();
}
