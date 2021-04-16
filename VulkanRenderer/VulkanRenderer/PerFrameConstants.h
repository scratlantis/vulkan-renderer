#pragma once
#include <glm/glm.hpp>
struct PerFrameConstants
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 inverseModel;
	glm::mat4 inverseView;
	glm::mat4 inverseProjection;
	glm::vec4 lightPosition;
	glm::vec4 lightPositionModel;
	glm::vec4 lightColor;
	glm::vec4 camPos;
	glm::vec4 camPosModel;

	glm::uint counter;
	glm::uint decalCount;
	glm::uint width;
	glm::uint height;
};