#pragma once
#include "PointLightData.h";
#include "DecalData.h"
struct Cursor
{
	DecalData decal;
	PointLightData light;
	glm::uint selection;
	glm::uint placeholder1;
	glm::uint placeholder2;
	glm::uint placeholder3;
};