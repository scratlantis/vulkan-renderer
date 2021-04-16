#version 460
#extension GL_GOOGLE_include_directive : enable

// So that the compiler doesnt complain
#ifndef TEXTURES
#define TEXTURES 42
#endif
#ifndef MATERIALS
#define MATERIALS 42
#endif
#define MAX_UINT 0xFFFFFFFF

#include "ubo_def.glsl"
#include "material_def.glsl"
layout(location = 0) flat in uint primitiveIndex;


layout (location = 0) out uint outInstaceId8triangleId24;


layout(binding = 0) uniform UBO_DATA
{
	UBO data;
} ubo;


layout(binding = 1) uniform sampler2D tex[TEXTURES];



layout(std140, binding = 2) uniform MATERIAL_DATA
{
	MaterialData matData[MATERIALS];
} mat;

void main()
{
	outInstaceId8triangleId24 = primitiveIndex; // TODO add instance index later
}