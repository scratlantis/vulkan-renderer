#version 460
#extension GL_GOOGLE_include_directive : enable



// So that the compiler doesnt complain
#ifndef TEXTURES
#define TEXTURES 42
#endif
#ifndef MATERIALS
#define MATERIALS 42
#endif
#ifndef DECALS
#define DECALS 42
#endif


#define MAX_UINT 0xFFFFFFFF
#include "ubo_def.glsl"
#include "material_def.glsl"
#include "decal_def.glsl"

layout(location = 0) flat in uint decalIndex;

layout(location = 1) flat in uint triangleIndex;

layout(location = 2) in VS_INTERPOLATET
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

layout (input_attachment_index = 0, binding = 4) uniform subpassInput samplerDepth;
layout(early_fragment_tests) in;
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

layout(std140, binding = 3) buffer DECAL_DATA
{
	DecalData decData[DECALS];
} dec;

layout(early_fragment_tests) in;

//layout (input_attachment_index = 2, binding = 4) uniform subpassInput samplerDepth;

vec4 calculateWorldPosition(float depth)
{
	vec2 uv = vec2(gl_FragCoord.x/ubo.data.width, gl_FragCoord.y/ubo.data.height);
	vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0f, depth, 1.0);
	vec4 viewSpacePosition = ubo.data.inverseProjection* clipSpacePosition;
	viewSpacePosition /= viewSpacePosition.w;
	vec4 worldSpacePosition = ubo.data.inverseView * viewSpacePosition;


	//return clipSpacePosition;
	//return viewSpacePosition;
	return worldSpacePosition;
}


bool applySingleDecal(uint idx, vec3 worldPos, vec3 dx_world , vec3 dy_world)
{
	// Get Position in decal coordinate system
	DecalData d = dec.decData[idx];
	vec4 pos_dec = d.inverseTransformation*vec4(worldPos, 1.0);
	//outAlbedo = vec4(0.0,0.0,1.0,0.8);//Debug
	//if(triangleIndex<6)outAlbedo = vec4(1.0,0.0,0.0,5.0);
	//return true;//Debug
	// Is fragment contained in decal OBB
	if(pos_dec.x > 1 || pos_dec.y > 1 ||pos_dec.z > 1 ||
	pos_dec.x < 0 || pos_dec.y < 0 || pos_dec.z < 0)
	{
		return false;
	}
	// Bilinear interpolation to get uv coords
	vec2 uv1 = mix(d.uvCoordAB.zw, d.uvCoordAB.xy, pos_dec.x);
	vec2 uv2 = mix(d.uvCoordCD.xy, d.uvCoordCD.zw, pos_dec.x);
	vec2 uv = mix(uv1, uv2, pos_dec.y);
	// Transform Gradients
	vec2 dx =  (d.inverseTransformation*vec4(dx_world,0.0)).xy;
	vec2 dy =  (d.inverseTransformation*vec4(dy_world,0.0)).xy;
	// Get material
	MaterialData materialData = mat.matData[d.materialIndex];
	// Fetch textures
	vec4 decalColor;
	if(materialData.idx_map_diffuse != MAX_UINT) {
		decalColor			= textureGrad(tex[materialData.idx_map_diffuse], uv, dx, dy);
	}else {
		decalColor = vec4(0.0,0.0,0.0,0.0);
	}
	vec4 decalNormal;
	if(materialData.idx_map_normal != MAX_UINT) {
		vec4 normalTexture	= textureGrad(tex[materialData.idx_map_normal], uv, dx, dy);
		decalNormal			= vec4(2*normalTexture.xyz - 1.0, normalTexture.a);
		//decalNormal.xyz		= normalize(((data.TNB * decalNormal.xyz)).rgb);
		decalNormal.z *= -1; // Because texture is beeing projected 
		decalNormal.x *= -1; // Because texture is beeing projected 
		decalNormal.xyz = normalize(((d.inverseTransposedTransformation * vec4(decalNormal.xyz,0.0))).rgb);
	}else {
		decalNormal = vec4(0.0,0.0,0.0,0.0);
	}
	vec4 specularData;
	if(materialData.idx_map_specular != MAX_UINT) {
		specularData		= textureGrad(tex[materialData.idx_map_specular], uv, dx, dy);
	}else {
		specularData = vec4(0.0,0.0,0.0,0.0);
	}
	//outAlbedo = vec4(1.0,0.0,0.0,1.0);
	//return true;

	outAlbedo = decalColor;
	outNormal = vec4(0.5*decalNormal.rgb + 0.5,decalNormal.a);
	//outNormal.a *= 0.2;
	outRoughnessMetalicity.r = specularData.g;
	outRoughnessMetalicity.g = specularData.b;
	outRoughnessMetalicity.a = specularData.a;
	return true;
}

void main()
{
	float depth = subpassLoad(samplerDepth).r;
	outNormal = vec4(0.0, 0.0, 0.0, 0.0);
	outAlbedo = vec4(0.0, 0.0, 0.0, 0.0);
	outRoughnessMetalicity = vec4(0.0, 0.0, 0.0, 0.0);
	if(depth<gl_FragCoord.z) return;
	vec4 worldPos = calculateWorldPosition(depth);
	vec3 dx_world     = dFdx(worldPos.xyz);
    vec3 dy_world     = dFdy(worldPos.xyz);
	applySingleDecal(decalIndex, worldPos.xyz, dx_world, dy_world);
	//if(!applySingleDecal(decalIndex, worldPos.xyz, dx_world, dy_world))
	//{
	//	outAlbedo = vec4(210.0/255.0, 34.0/255.0, 45.0/255.0, 1.0);
	//} else
	//{
	//	//outAlbedo = vec4(1.0, 0.0, 0.0, 1.0);
	//	outAlbedo = vec4(0.0, 0.0, 0.0, 0.0);
	//}

}