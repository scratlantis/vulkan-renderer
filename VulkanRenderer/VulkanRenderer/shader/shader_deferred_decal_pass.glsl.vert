#version 460
#extension GL_GOOGLE_include_directive : enable
#ifndef DECALS
#define DECALS 42
#endif

#include "ubo_def.glsl"
#include "decal_def.glsl"

out gl_PerVertex {
	vec4 gl_Position;
};



layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 inUVCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;
layout(location = 6) in uint decalIndex;


//struct DecalData
//{
//	
//	mat4 transformation;
//    mat4 inverseTransformation;
//    vec4 dimensions;
//
//    vec4 uvCoordA;
//    vec4 uvCoordB;
//    vec4 uvCoordC;
//    vec4 uvCoordD;
//
//	uint materialIndex;
//	float weight;
//	uint placeholder2;
//	uint placeholder3;
//
//
//};

layout(binding = 0) uniform UBO_DATA
{
	UBO data;
} ubo;
layout(std140, binding = 3) buffer DECAL_DATA
{
	DecalData decData[DECALS];
} dec;

layout(location = 1) flat out uint triangleIndex;
layout(location = 2) out VS_INTERPOLATET
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

layout(location = 0) flat out uint fragDecalIndex;






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





void main()
{
	gl_Position = ubo.data.projection * ubo.data.view * ubo.data.model * vec4(pos, 1.0);
	fragDecalIndex = decalIndex;
	DecalData d = dec.decData[fragDecalIndex];
	vec4 pos_dec = d.inverseTransformation*vec4(ubo.data.camPos.xyz,1.0);
	triangleIndex = gl_VertexIndex%36;
	if(!(pos_dec.x > 1 || pos_dec.y > 1 ||pos_dec.z > 1 ||
	pos_dec.x < 0 || pos_dec.y < 0 || pos_dec.z < 0))
	{
		uint decalVertexIndex = gl_VertexIndex%36; // 3 Vertecies per triangle * 2 triangle per face * 6 faces
		if (decalVertexIndex < 3)
		{
			vec2 uv = vec2(decalVertexIndex & 2,(decalVertexIndex << 1) & 2);
			gl_Position = vec4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
		}
	}
	

}
