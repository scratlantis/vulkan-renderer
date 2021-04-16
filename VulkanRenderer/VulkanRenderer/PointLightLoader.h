#pragma once
#include <glm/glm.hpp>
#include "Node.h"


#define EPSIOLON 0.01

#define MIN_LIGHT_THRESHOLD (5.0/256.0)

class PointLightLoader
{
private:

public:
	PointLightLoader();
	~PointLightLoader();


	float getLightSphereRadious(float intensity);

	void loadPointLight(tinyobj::shape_t shape, tinyobj::attrib_t* vertexAttributes, Node& node);

	void generatePointLight(glm::vec3 position, Node& node);
	void generatePointLight(glm::vec3 position, glm::vec4 color, Node& node);

};




