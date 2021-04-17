#version 460
//#extension GL_ARB_seperate_objects : enable
//#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable
//#extension GL_EXT_ray_tracing : enable
//#extension GL_EXT_scalar_block_layout : enable
//#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
//#extension GL_EXT_ray_flags_primitive_culling : enable

// Implementation of Frostbite BRDF as described in
//https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

#ifndef TEXTURES
#define TEXTURES 42
#endif
#ifndef MATERIALS
#define MATERIALS 42
#endif
#ifndef DECALS
#define DECALS 42
#endif
#ifndef POINT_LIGHTS
#define POINT_LIGHTS 1
#endif




#ifdef ENABLE_RAY_QUERY_LIGHTS
#define USE_RAY_QUERY_LIGHTS true
#else
#define USE_RAY_QUERY_LIGHTS false
#endif

#ifdef ENABLE_PRIMARY_POINT_LIGHT
#define APPLY_PRIMARY_POINT_LIGHT applyPrimaryPointLight(outColor, data);
#else
#define APPLY_PRIMARY_POINT_LIGHT
#endif


// If we dont use ray querys at all the compiler will do an optimization
// that will cause an error on the cpp side. To fix this properly we would have to
// completely remove all ray query related code. And i dont feel like putting 500+ lines
// of codes in defines :P. Therefor we just use a ray query operation that
// is never actually executed. Hence: if(ubo.data.counter == 1337)applyDecals(data);
// This is still uniform control flow!
#ifdef ENABLE_RAY_QUERY_DECALS
#define APPLY_RAY_QUERY_DECALS applyDecals(data);
#else
#define APPLY_RAY_QUERY_DECALS if(ubo.data.counter == 1337)applyDecals(data);
#endif

#ifdef ENABLE_RAY_QUERY_DECAL_AABBS
#define APPLY_RAY_QUERY_DECAL_AABBS visualizeAABB(outColor, data);;
#else
#define APPLY_RAY_QUERY_DECAL_AABBS
#endif

#ifdef ENABLE_RAY_QUERY_SHADOWS
#define APPLY_RAY_QUERY_SHADOWS castShadowRay(outColor, data);;
#else
#define APPLY_RAY_QUERY_SHADOWS
#endif

#ifdef ENABLE_AMBIENT_TERM
#define APPLY_AMBIENT_TERM outColor.rgb += 0.002 * data.diffuse_color.rgb;
#else
#define APPLY_AMBIENT_TERM
#endif

#ifdef ENABLE_SCENE_POINT_LIGHTS
#define APPLY_SCENE_POINT_LIGHTS applyPointLights(outColor, data);
#else
#define APPLY_SCENE_POINT_LIGHTS
#endif

#ifdef ENABLE_RAY_QUERY_REFLECTIONS
#define APPLY_RAY_QUERY_REFLECTIONS
#else
#define APPLY_RAY_QUERY_REFLECTIONS
#endif

#ifdef ENABLE_EMISSIVE_TERM
#define APPLY_EMISSIVE_TERM outColor.rgb += 2.0*data.emissive.rgb;
#else
#define APPLY_EMISSIVE_TERM
#endif





//#ifdef ENABLE_RAY_QUERY_DECALS
//#define LOAD_AS_DECALS layout(binding = 4, set = 0) uniform accelerationStructureEXT topLevelAS;
//#else
//#define LOAD_AS_DECALS
//#endif
//
//#ifdef ENABLE_RAY_QUERY_SHADOWS
//#define LOAD_AS_SCENE layout(binding = 5, set = 0) uniform accelerationStructureEXT topLevelASScene;
//#else
//#define LOAD_AS_SCENE
//#endif
//
//
//#ifdef ENABLE_RAY_QUERY_LIGHTS
//#define LOAD_AS_LIGHTS layout(binding = 7, set = 0) uniform accelerationStructureEXT topLevelASPointLights;
//#else
//#define LOAD_AS_LIGHTS
//#endif




#define BLENDING_CHANNELS 4
#define EPSILON 0.01
#define MAX_UINT 0xFFFFFFFF

#include "ubo_def.glsl"
#include "material_def.glsl"
#include "decal_def.glsl"
#include "shading_data_def.glsl"
#include "point_light_def.glsl"
#include "brdf.glsl"


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
layout(binding = 0) uniform UBO_DATA
{
	UBO data;
} ubo;
layout(binding = 1) uniform sampler2D tex[TEXTURES];
layout(std140, binding = 2) uniform MATERIAL_DATA
{
	MaterialData matData[MATERIALS];
} mat;
layout(std140, binding = 3) buffer DECAL_DATA
{
	DecalData decData[DECALS];
} dec;
//layout(std140, binding = 6) uniform POINT_LIGHT_DATA
//{
//	PointLightData data[POINT_LIGHTS];
//} pointLights;
layout(std140, binding = 6) buffer POINT_LIGHT_DATA 
{
   PointLightData data[POINT_LIGHTS];
}pointLights;

//layout (local_size_x = 256) in;



layout(binding = 4, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 5, set = 0) uniform accelerationStructureEXT topLevelASScene;
layout(binding = 7, set = 0) uniform accelerationStructureEXT topLevelASPointLights;

//LOAD_AS_DECALS
//LOAD_AS_SCENE
//LOAD_AS_LIGHTS

layout(location = 0) out vec4 outColor;
layout(early_fragment_tests) in;

//######################## Lighting ##########################

void calculateShadingVectors(inout ShadingData data, vec3 lightPosition)
{
	data.view			= normalize(ubo.data.camPos.xyz - data.worldPos);
	data.lightVector	=  normalize(lightPosition - data.worldPos);
	data.halfWayVector	= normalize(data.lightVector + data.view);
	data.dotLH			= clamp(dot(data.lightVector, data.halfWayVector), 0.0, 1.0);
	data.dotNV			= abs(dot(data.normal, data.view) + 0.0001);
	data.dotNH			= clamp(dot(data.normal, data.halfWayVector), 0.0, 1.0);
	data.dotNL			= clamp(dot(data.normal, data.lightVector), 0.0, 1.0);
}
ShadingData prepareShadingData()
{
	vec3 N = normalize(vs_out.fragNormal);
	vec3 T = normalize(vs_out.fragTangent);
	T = normalize(T - dot(T, N) * N);
	vec3 B = -normalize(cross(N, T));

	uint idx = materialIndex;
	ShadingData data;
	data.TNB = mat3(T,B,N);
	data.normal			= normalize(data.TNB*(2.0 * (texture(tex[mat.matData[idx].idx_map_normal], vs_out.uvCoord).rgb) - 1.0));
	vec3 specularData = texture(tex[mat.matData[idx].idx_map_specular], vs_out.uvCoord).rgb;
	data.metalicity = specularData.b;
	data.specular_color = vec3(1.0);
	data.diffuse_albedo  = texture(tex[mat.matData[idx].idx_map_diffuse], vs_out.uvCoord).rgb;
	data.fresnel_0		= mix(vec3(0.02f), data.diffuse_albedo, data.metalicity);
	data.diffuse_color = (1.0 - data.metalicity)*data.diffuse_albedo;
	if(mat.matData[materialIndex].idx_map_emissive != MAX_UINT)
	{
		data.emissive =  texture(tex[mat.matData[idx].idx_map_emissive], vs_out.uvCoord).rgb;
	}
	else
	{
		data.emissive = vec3(0.0);
	}
	data.roughness		= specularData.g;
	data.PI				= 3.141592f;
	data.modelPos = vs_out.modelPos.xyz;
	data.worldPos = vs_out.worldPos.xyz;
	calculateShadingVectors(data, ubo.data.lightPosition.xyz);
	return data;
}




//######################## Ray Querys ##########################
void castShadowRay(inout vec4 outColor, ShadingData data)
{
	vec3 origin = data.worldPos;
	vec3 direction = normalize(ubo.data.lightPosition.rgb - data.worldPos);
	float tMin = 0.001;
	float tMax = length(ubo.data.lightPosition.rgb - data.worldPos);
	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, topLevelASScene, gl_RayFlagsOpaqueEXT, 0xFF, origin, tMin,direction, tMax);
	while(rayQueryProceedEXT(rayQuery)){}
	bool hit = (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT);
	if(hit)
	{
		outColor.rgb *= 0.2;
	}
}


void blendDecal(vec4 color, vec4 normal, vec4 specularData, float weight, inout DecalBlendingData blendingData)
{
	// Texture Blending
	blendingData.color.rgb				+= color.rgb * color.a * weight;
	blendingData.color.a				*= 1.0-color.a;
	blendingData.weight_color			+= color.a * weight;
	// Normal Blending
	blendingData.normal.rgb				+= normal.rgb * weight * normal.a;
	blendingData.normal.a				*= 1.0-normal.a;
	blendingData.weight_normal			+= weight * normal.a;
	// Roughness & Metalicity
	blendingData.specularData.rgb		+= specularData.rgb * weight * specularData.a;
	blendingData.specularData.a			*= 1.0-specularData.a;
	blendingData.weight_specularData	+= specularData.a * weight;
}

bool applySingleDecal(inout DecalBlendingData blendingData[BLENDING_CHANNELS], uint idx, ShadingData data, vec3 dx_world, vec3 dy_world)
{
	// Get Position in decal coordinate system
	DecalData d = dec.decData[idx];
	vec4 pos_dec = d.inverseTransformation*vec4(data.worldPos, 1.0);
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
	//MaterialData materialData = mat.matData[4];
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
		vec3 N = normalize(vs_out.fragNormal);
		vec3 T = d.inverseTransposedTransformation[0].xyz;
		T = -normalize(T-N*dot(N,T));
		vec3 B = -normalize(cross(N, T));
		mat3 TBN = mat3(T,B,N);
		decalNormal.xyz		= normalize(((TBN * decalNormal.xyz)).rgb);
		//decalNormal.xyz = normalize(((d.inverseTransposedTransformation * vec4(decalNormal.xyz,0.0))).rgb);
		//decalNormal.z *= -1; // Because texture is beeing projected 
	}else {
		decalNormal = vec4(0.0,0.0,0.0,0.0);
	}
	vec4 specularData;
	if(materialData.idx_map_specular != MAX_UINT) {
		specularData		= textureGrad(tex[materialData.idx_map_specular], uv, dx, dy);
	}else {
		specularData = vec4(0.0,0.0,0.0,0.0);
	}
	// Per channel blending
	if(d.channel == 0)		blendDecal(decalColor, decalNormal, specularData, d.weight, blendingData[0]);
	else if(d.channel == 1) blendDecal(decalColor, decalNormal, specularData, d.weight, blendingData[1]);
	else if(d.channel == 2) blendDecal(decalColor, decalNormal, specularData, d.weight, blendingData[2]);
	else if(d.channel == 3) blendDecal(decalColor, decalNormal, specularData, d.weight, blendingData[3]);
	return true;
}


void applyDecals(inout ShadingData data)
{
	DecalBlendingData blendingData[BLENDING_CHANNELS];
	blendingData[0].color = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[0].normal = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[0].specularData = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[0].weight_color = 0.0;
	blendingData[0].weight_normal = 0.0;
	blendingData[0].weight_specularData = 0.0;

	blendingData[1].color = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[1].normal = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[1].specularData = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[1].weight_color = 0.0;
	blendingData[1].weight_normal = 0.0;
	blendingData[1].weight_specularData = 0.0;

	blendingData[2].color = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[2].normal = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[2].specularData = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[2].weight_color = 0.0;
	blendingData[2].weight_normal = 0.0;
	blendingData[2].weight_specularData = 0.0;

	blendingData[3].color = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[3].normal = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[3].specularData = vec4(0.0, 0.0, 0.0, 1.0);
	blendingData[3].weight_color = 0.0;
	blendingData[3].weight_normal = 0.0;
	blendingData[3].weight_specularData = 0.0;

	// Calculate world space derivatives:
	vec3 dx_vtc     = dFdx(vs_out.worldPos.xyz);
    vec3 dy_vtc     = dFdy(vs_out.worldPos.xyz);

	// Ray Query Loop
	vec3 origin = data.worldPos;
	vec3 direction = vec3(1.0, 0.0, 0.0);
	float tMin = 0.0;
	float tMax = 0.0;
	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, topLevelAS,
			gl_RayFlagsSkipClosestHitShaderEXT
			| gl_RayFlagsOpaqueEXT
			| gl_RayFlagsCullBackFacingTrianglesEXT
			| gl_RayFlagsCullFrontFacingTrianglesEXT,
			0xFF, origin, tMin, direction, tMax);
	while(rayQueryProceedEXT(rayQuery))
	{
		applySingleDecal(blendingData,
			rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false),
			data, dx_vtc, dy_vtc);
	}

	// Blend Levels
	vec4 blending_color;
	vec4 blending_normal;
	vec4 blending_specularData;	

	blendingData[0].weight_color =			max(EPSILON, blendingData[0].weight_color);
	blendingData[0].weight_normal =			max(EPSILON, blendingData[0].weight_normal);
	blendingData[0].weight_specularData =	max(EPSILON, blendingData[0].weight_specularData);

	blendingData[1].weight_color =			max(EPSILON, blendingData[1].weight_color);
	blendingData[1].weight_normal =			max(EPSILON, blendingData[1].weight_normal);
	blendingData[1].weight_specularData =	max(EPSILON, blendingData[1].weight_specularData);

	blendingData[2].weight_color =			max(EPSILON, blendingData[2].weight_color);
	blendingData[2].weight_normal =			max(EPSILON, blendingData[2].weight_normal);
	blendingData[2].weight_specularData =	max(EPSILON, blendingData[2].weight_specularData);

	blendingData[3].weight_color =			max(EPSILON, blendingData[3].weight_color);
	blendingData[3].weight_normal =			max(EPSILON, blendingData[3].weight_normal);
	blendingData[3].weight_specularData =	max(EPSILON, blendingData[3].weight_specularData);


	blending_color.rgb =		blendingData[0].color.rgb		/	blendingData[0].weight_color;
	blending_normal.rgb =		blendingData[0].normal.rgb		/	blendingData[0].weight_normal;
	blending_specularData.rgb = blendingData[0].specularData.rgb/	blendingData[0].weight_specularData;

	blending_color.a			= 1.0-blendingData[0].color.a;
	blending_normal.a			= 1.0-blendingData[0].normal.a;
	blending_specularData.a		= 1.0-blendingData[0].specularData.a;

	//blending_color.rgb = vec3(0.1+blendingData[0].color.a);

	blending_normal.rgb = normalize(blending_normal.rgb);
	if(blendingData[0].weight_color>EPSILON)data.diffuse_albedo.rgb =	mix(data.diffuse_albedo.rgb, blending_color.rgb, blending_color.a);
	if(blendingData[0].weight_normal>EPSILON)data.normal =				mix(data.normal, blending_normal.rgb, blending_normal.a);
	if(blendingData[0].weight_specularData>EPSILON)
	{
		data.metalicity =												mix(data.metalicity, blending_specularData.b, blending_specularData.a);
		data.roughness =												mix(data.roughness, blending_specularData.g, blending_specularData.a);
	}

	blending_color =		blendingData[1].color		/	blendingData[1].weight_color;
	blending_normal =		blendingData[1].normal		/	blendingData[1].weight_normal;
	blending_specularData = blendingData[1].specularData/	blendingData[1].weight_specularData;

	blending_color.a			= 1.0-blendingData[1].color.a;
	blending_normal.a			= 1.0-blendingData[1].normal.a;
	blending_specularData.a		= 1.0-blendingData[1].specularData.a;

	blending_normal.rgb = normalize(blending_normal.rgb);
	if(blendingData[1].weight_color>EPSILON)data.diffuse_albedo.rgb =	mix(data.diffuse_albedo.rgb, blending_color.rgb, blending_color.a);
	if(blendingData[1].weight_normal>EPSILON)data.normal =				mix(data.normal, blending_normal.rgb, blending_normal.a);
	if(blendingData[1].weight_specularData>EPSILON)
	{
		data.metalicity =												mix(data.metalicity, blending_specularData.b, blending_specularData.a);
		data.roughness =												mix(data.roughness, blending_specularData.g, blending_specularData.a);
	}

	blending_color =		blendingData[2].color		/	blendingData[2].weight_color;
	blending_normal =		blendingData[2].normal		/	blendingData[2].weight_normal;
	blending_specularData = blendingData[2].specularData/	blendingData[2].weight_specularData;
	blending_color.a			= 1.0-blendingData[2].color.a;
	blending_normal.a			= 1.0-blendingData[2].normal.a;
	blending_specularData.a		= 1.0-blendingData[2].specularData.a;
	blending_normal.rgb = normalize(blending_normal.rgb);
	if(blendingData[2].weight_color>EPSILON)data.diffuse_albedo.rgb =	mix(data.diffuse_albedo.rgb, blending_color.rgb, blending_color.a);
	if(blendingData[2].weight_normal>EPSILON)data.normal =				mix(data.normal, blending_normal.rgb, blending_normal.a);
	if(blendingData[2].weight_specularData>EPSILON)
	{
		data.metalicity =												mix(data.metalicity, blending_specularData.b, blending_specularData.a);
		data.roughness =												mix(data.roughness, blending_specularData.g, blending_specularData.a);
	}

	blending_color =		blendingData[0].color		/	blendingData[3].weight_color;
	blending_normal =		blendingData[0].normal		/	blendingData[3].weight_normal;
	blending_specularData = blendingData[0].specularData/	blendingData[3].weight_specularData;
	blending_color.a			= 1.0-blendingData[3].color.a;
	blending_normal.a			= 1.0-blendingData[3].normal.a;
	blending_specularData.a		= 1.0-blendingData[3].specularData.a;
	blending_normal.rgb = normalize(blending_normal.rgb);
	if(blendingData[3].weight_color>EPSILON)data.diffuse_albedo.rgb =	mix(data.diffuse_albedo.rgb, blending_color.rgb, blending_color.a);
	if(blendingData[3].weight_normal>EPSILON)data.normal =				mix(data.normal, blending_normal.rgb, blending_normal.a);
	if(blendingData[3].weight_specularData>EPSILON)
	{
		data.metalicity =												mix(data.metalicity, blending_specularData.b, blending_specularData.a);
		data.roughness =												mix(data.roughness, blending_specularData.g, blending_specularData.a);
	}

	data.normal			=		normalize(data.normal);
	data.fresnel_0		=	mix(vec3(0.02f), data.diffuse_albedo, data.metalicity);
	data.diffuse_color	= (1.0 - data.metalicity)*data.diffuse_albedo;
	//calculateShadingVectors(data, ubo.data.lightPosition.xyz);
	
}
void visualizeAABB(inout vec4 outColor, ShadingData data)
{
	vec3 origin = data.worldPos;
	vec3 direction = normalize(ubo.data.camPos.rgb - data.worldPos);
	float tMin = 0.0;
	float tMax = length(ubo.data.camPos.rgb - data.worldPos);
	bool query = false;
	bool hit;
	outColor.a = 1.0;
	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, origin, tMin, direction, tMax);
	vec3 boxColor = vec3(0.0,0.0,0.0);
	uint boxCount = 0;
	while(rayQueryProceedEXT(rayQuery))
	{
		hit = (rayQueryGetIntersectionCandidateAABBOpaqueEXT(rayQuery));
		if(hit)
		{
			int index = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
			//boxColor += 0.2*vec3(index%5, (index-5)%5, (index-10)%5);
			boxColor += vec3(0.1*(index%10), 0.1*((index/10)%10),  0.1*((index/100)%10));
			boxCount++;
		}
	}
	outColor.rgb = (outColor.rgb + boxColor)/(boxCount+1.0);
	
}


float convertLinearToSRGB(float linearChannel)
{
	linearChannel = clamp(linearChannel, 0.0f, 1.0f);
	if (linearChannel <= 0.0031308f)
	{
		return 12.92 * linearChannel;
	} else
	{
		return 1.055 * pow(linearChannel, 1.0f / 2.4f) - 0.055f;
	}
}

void convertLinearToSRGB(inout vec4 outColor)
{
	outColor.r = convertLinearToSRGB(outColor.r);
	outColor.g = convertLinearToSRGB(outColor.g);
	outColor.b = convertLinearToSRGB(outColor.b);
}

float calculateLightIntesity(float lightDistance)
{
	float attenuation_CONST = 0.0;
	float attenuation_LIN = 0.0;
	float attenuation_EXP = 1.0;

	float lightDistance2 = lightDistance * lightDistance;
	return 1.0 / (attenuation_CONST + attenuation_LIN*lightDistance + attenuation_EXP*lightDistance2);
}

vec3 applySinglePointLight(ShadingData data, PointLightData pointLight)
{
	vec3 lightPosition = pointLight.position.xyz;//+ vec3(0.0,0.0,0.2);
	calculateShadingVectors(data, lightPosition);
	vec3 brdf = evaluateBRDF(data);
	float lightIntensity = calculateLightIntesity(length(lightPosition - data.worldPos.xyz))* pointLight.color.a;
	return brdf*data.dotNL*lightIntensity * pointLight.color.rgb;
}

void applyPointLights(inout vec4 outColor, ShadingData data)
{
	// loop over all point lights
	if(USE_RAY_QUERY_LIGHTS)
	{
		vec3 origin = data.worldPos;
		vec3 direction = vec3(1.0, 0.0, 0.0);
		float tMin = 0.0;
		float tMax = 0.0;
		bool query = false;
		bool hit;
		rayQueryEXT rayQuery;
		rayQueryInitializeEXT(rayQuery, topLevelASPointLights,
		gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsCullBackFacingTrianglesEXT | gl_RayFlagsCullFrontFacingTrianglesEXT,
		0xFF, origin, tMin, direction, tMax);

		while(rayQueryProceedEXT(rayQuery))
		{
			outColor.rgb += applySinglePointLight(data, pointLights.data[rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false)]);
		}
	} else {
		for(int i = 0; i<POINT_LIGHTS;i++)
		{
			outColor.rgb += applySinglePointLight(data, pointLights.data[i]);
		}
	}
}

void applyPrimaryPointLight(inout vec4 outColor, ShadingData data)
{
	vec3 lightPosition = ubo.data.lightPosition.xyz;
	calculateShadingVectors(data, lightPosition);
	vec3 brdf = evaluateBRDF(data);
	float lightIntensity = calculateLightIntesity(length(ubo.data.lightPosition.xyz - data.worldPos.xyz)) * ubo.data.lightColor.a;
	outColor.rgb += brdf*data.dotNL*lightIntensity * ubo.data.lightColor.rgb;
}







void main()
{
	ShadingData data = prepareShadingData();
	outColor = vec4(0.0, 0.0, 0.0, 1.0);
	APPLY_RAY_QUERY_DECALS
	APPLY_PRIMARY_POINT_LIGHT
	APPLY_RAY_QUERY_SHADOWS
	APPLY_SCENE_POINT_LIGHTS
	APPLY_RAY_QUERY_REFLECTIONS
	APPLY_AMBIENT_TERM
	APPLY_EMISSIVE_TERM
	APPLY_RAY_QUERY_DECAL_AABBS
	convertLinearToSRGB(outColor);
	//outColor.rgb = 0.5*data.normal.rgb + 0.5;
}
