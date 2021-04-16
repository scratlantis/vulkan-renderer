#pragma once
#include <glm/glm.hpp>
struct DecalData
{
	glm::mat4 inverseTransposedTransformation;
	glm::mat4 inverseTransformation;

	glm::vec4 uvCoordAB;
	glm::vec4 uvCoordCD;

	glm::uint materialIndex;
	glm::float32 weight;
	glm::uint channel;
	glm::uint placeholder3;
};