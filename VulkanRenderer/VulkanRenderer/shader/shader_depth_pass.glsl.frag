#version 460
#extension GL_GOOGLE_include_directive : enable

#ifndef TEXTURES
#define TEXTURES 42
#endif
#ifndef MATERIALS
#define MATERIALS 42
#endif
#define MAX_UINT 0xFFFFFFFF

#include "material_def.glsl"

layout(binding = 1) uniform sampler2D tex[TEXTURES];

layout(location = 0) flat in uint materialIndex;


layout(location = 1) in VS_INTERPOLATET
{
	vec3 viewVector;
	vec2 uvCoord;
	vec3 lightVector;
	vec4 worldPos;

	vec3 fragNormal;
	vec3 fragTangent;
	vec3 fragBitangent;
	vec4 modelPos;

} vs_out;

//struct MaterialData
//{
//	vec4 ambient;
//	vec4 diffuse;
//	vec4 specular;
//
//	//float specularExponent[MATERIALS];
//
//	uint slotUsed;
//	uint idx_map_specular;
//	uint idx_map_diffuse;
//	uint idx_map_normal;
//
//	uint idx_map_alpha;
//	uint placeholder1;
//	uint placeholder2;
//	uint placeholder3;
//
//
//};

layout(std140, binding = 2) uniform MATERIAL_DATA
{
	MaterialData matData[MATERIALS];
} mat;

void main()
{
	if(mat.matData[materialIndex].idx_map_alpha != MAX_UINT)
		{

			if(texture(tex[mat.matData[materialIndex].idx_map_alpha], vs_out.uvCoord).a < 0.50)
			{
				discard;
			}
		}
}