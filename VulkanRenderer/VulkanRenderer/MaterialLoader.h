#pragma once




#include "VulkanUtils.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include "TextureImage.h"

using namespace VulkanUtils;

struct MaterialProperties
{
	glm::f32vec3 ambient;		//Ka
	glm::f32vec3 diffuse;		//Kd
	glm::f32vec3 specular;		//Ks
	glm::f32vec3 emissive;		//Ke

	glm::float32 specularExponent;		//Ns
	glm::float32 transparency;			//Ts

	//Texture map indices
	glm::uint idx_map_specular;
	glm::uint idx_map_diffuse;
	glm::uint idx_map_normal;
	glm::uint idx_map_alpha;
	glm::uint idx_map_emissive;
};


class MaterialLoader {

private:
	std::unordered_map<std::string, int> m_textures_map;
	std::vector<std::string> m_textures_list;
	std::vector<bool> m_useSRGB;
	std::vector<MaterialProperties> m_materialProperties;
	std::vector<int> m_decalMaterialIndices;


public:
	MaterialLoader();

	void load(std::vector<tinyobj::material_t> materials, std::string baseDir);

	void load_Decals(std::string baseDir);

	void uploadMaterials(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue queue,
		std::vector<TextureImage*>* images);

	std::vector<MaterialProperties> getMaterialProperties();

	std::vector<int> getDecalMaterialIndices();

	size_t getTextureCount();

	size_t getMaterialCount();
};
