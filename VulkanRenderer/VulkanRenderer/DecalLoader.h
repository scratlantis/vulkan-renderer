#pragma once

#include "Node.h"

class DecalLoader
{
private:

public:
	DecalLoader();
	~DecalLoader();



	void loadDecal(tinyobj::shape_t shape, tinyobj::attrib_t* vertexAttributes, Node& node);
	void generateDecal(glm::mat4 transformation, uint32_t materialIndex, Node& node, uint32_t weight, uint32_t channel);
	void generateDecal(glm::vec3 position, glm::vec3 normal, float scale, uint32_t materialIndex, Node& node);

};



