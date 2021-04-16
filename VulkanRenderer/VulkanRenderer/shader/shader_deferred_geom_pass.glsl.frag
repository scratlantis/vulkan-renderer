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

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outRoughnessMetalicity;


//layout(binding = 0) uniform UBO
//{
//	mat4 model;
//	mat4 view;
//	mat4 projection;
//	mat4 inverseModel;
//	mat4 inverseView;
//	mat4 inverseProjection;
//	vec4 lightPosition;
//	vec4 lightPositionModel;
//	vec4 lightColor;
//	vec4 camPos;
//	vec4 camPosModel;
//	uint counter;
//	uint decalCount;
//	uint width;
//	uint height;
//} ubo;
layout(binding = 0) uniform UBO_DATA
{
	UBO data;
} ubo;

layout(binding = 1) uniform sampler2D tex[TEXTURES];


//struct MaterialData
//{
//	vec4 ambient;
//	vec4 diffuse;
//	vec4 specular;
//
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


// TODO implement
vec3 calculateNormal()
{
	vec3 N = normalize(vs_out.fragNormal);
	vec3 T = normalize(vs_out.fragTangent);
	T = normalize(T - dot(T, N) * N);
	vec3 B = -normalize(cross(N, T));

	mat3 TBN = mat3(T,B,N);

	return normalize(TBN*(2.0 * (texture(tex[mat.matData[materialIndex].idx_map_normal], vs_out.uvCoord).rgb) - 1.0));
}

void main()
{
	MaterialData material = mat.matData[materialIndex];
	if(material.idx_map_alpha != MAX_UINT)
		{
	
			if(texture(tex[material.idx_map_alpha], vs_out.uvCoord).a < 0.50)
			{
				discard;
			}
		}
	vec4 specularData = texture(tex[material.idx_map_specular], vs_out.uvCoord);
	float metalicity = specularData.b;
	float roughness	 = specularData.g;
	outRoughnessMetalicity = vec4(roughness, metalicity, 0, 0);
	outAlbedo = texture(tex[material.idx_map_diffuse], vs_out.uvCoord);
	outNormal.rgb = 0.5*calculateNormal()+0.5;

	//Debug
	//outRoughnessMetalicity = vec4(0,0,0,0);
	//outAlbedo = vec4(0,0,0,0);
	//outNormal = vec4(0,0,0,0);



}