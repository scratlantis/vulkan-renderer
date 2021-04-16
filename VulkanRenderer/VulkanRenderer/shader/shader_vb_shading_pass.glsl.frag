#version 460
#extension GL_EXT_ray_query : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_EXT_samplerless_texture_functions : enable
#extension GL_GOOGLE_include_directive : enable

#define BLENDING_CHANNELS 4
#define EPSILON 0.01


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
#define APPLY_PRIMARY_POINT_LIGHT applyPrimaryPointLight(outColor, data, viewPosition, coefficiant);
#else
#define APPLY_PRIMARY_POINT_LIGHT
#endif

// If we dont use ray querys at all the compiler will do an optimization
// that will cause an error on the cpp side. To fix this properly we would have to
// completely remove all ray query related code. And i dont feel like putting 500+ lines
// of codes in defines :P. Therefor we just use a ray query operation that
// is never actually executed. Hence: if(ubo.data.counter == 1337)applyDecals(data); 
#ifdef ENABLE_RAY_QUERY_DECALS
#define APPLY_RAY_QUERY_DECALS applyDecals(data);
#else
#define APPLY_RAY_QUERY_DECALS if(ubo.data.counter == 1337)applyDecals(data);
#endif

#ifdef ENABLE_RAY_QUERY_DECAL_AABBS
#define APPLY_RAY_QUERY_DECAL_AABBS visualizeAABB(outColor, data);
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
#define APPLY_SCENE_POINT_LIGHTS applyPointLights(outColor, data, viewPosition, coefficiant);
#else
#define APPLY_SCENE_POINT_LIGHTS
#endif


#ifdef ENABLE_RAY_QUERY_REFLECTIONS
#define APPLY_RAY_QUERY_REFLECTIONS if(data.roughness<MAX_REFLECTION_ROUGHNESS||data.metalicity>0.5)applyReflection(outColor, data);
#else
#define APPLY_RAY_QUERY_REFLECTIONS
#endif

#ifdef ENABLE_EMISSIVE_TERM
#define APPLY_EMISSIVE_TERM outColor.rgb += data.emissive.rgb;
#else
#define APPLY_EMISSIVE_TERM
#endif

#ifdef ENABLE_EDITOR
#define APPLY_DECAL_EDITOR if(cursor.selection == 0)applyDecalCursor(blendingData, data, dx_vtc, dy_vtc);
#else
#define APPLY_DECAL_EDITOR
#endif

#ifdef ENABLE_EDITOR
#define APPLY_LIGHT_EDITOR if(cursor.selection == 1) applyEditorPointLight(outColor, data, viewPosition, coefficiant);
#else
#define APPLY_LIGHT_EDITOR
#endif


#define INFINITY 1000.0
#define MAX_REFLECTION_ROUGHNESS 0.09
#define MAX_UINT 0xFFFFFFFF
#define MAX_RECURSION 4

#include "ubo_def.glsl"
#include "material_def.glsl"
#include "decal_def.glsl"
#include "shading_data_def.glsl"
#include "point_light_def.glsl"
#include "brdf.glsl"

layout (input_attachment_index = 0, binding = 6) uniform usubpassInput samplerId;
layout(location = 0) in vec2 inUV;
layout(binding = 0) uniform UBO_DATA
{
	UBO data;
} ubo;

layout(location = 0) out vec4 outColor;


layout(binding = 1) uniform sampler2D tex[TEXTURES];

layout(std140, binding = 2) uniform MATERIAL_DATA
{
	MaterialData matData[MATERIALS];
} mat;

layout(std140, binding = 3) buffer DECAL_DATA
{
	DecalData decData[DECALS];
} dec;

layout(std140, binding = 7) buffer POINT_LIGHT_DATA 
{
   PointLightData data[POINT_LIGHTS];
}pointLights;


// Fix later
layout(binding = 4, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 5, set = 0) uniform accelerationStructureEXT topLevelASScene;
layout(binding = 8, set = 0) uniform accelerationStructureEXT topLevelASPointLights;

layout (binding = 9) uniform textureBuffer vertex_positions;
layout (binding = 10) uniform textureBuffer vertex_normals;
layout (binding = 11) uniform textureBuffer vertex_tex_coords;
layout (binding = 12) uniform textureBuffer vertex_tangents;
layout (binding = 13) uniform textureBuffer vertex_bitangents;
layout (binding = 14) uniform utextureBuffer vertex_matId;

layout (binding = 15) uniform CURSOR
{
	DecalData decal;
	PointLightData light;
	uint selection;
	uint placeholder1;
	uint placeholder2;
	uint placeholder3;
} cursor;

float linearize_depth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

struct TriangleData
{
	vec3 pos[3];
	vec3 normal[3];
	vec2 texCoords[3];
	vec3 tangent[3];
	vec3 bitangent[3];
	uint matId;
};

struct InterpolatedTriangleData
{
	vec3 pos;
	vec3 normal;
	vec2 texCoords;
	vec3 tangent;
	vec3 bitangent;
	uint matId;
};

struct Ray
{
	vec3 origin;
	vec3 direction;
};

void getTriangleData(uint triangleId, out TriangleData triangleData)
{
	for(int i = 0; i < 3; i++)
	{
		int vertexIndex = int(triangleId) * 3 + i;
		triangleData.pos[i] = texelFetch(vertex_positions, vertexIndex).rgb;
		triangleData.normal[i] = texelFetch(vertex_normals, vertexIndex).rgb;
		triangleData.texCoords[i] = texelFetch(vertex_tex_coords, vertexIndex).rg;
		triangleData.texCoords[i] = vec2(triangleData.texCoords[i].r, -triangleData.texCoords[i].g);
		triangleData.tangent[i] = texelFetch(vertex_tangents, vertexIndex).rgb;
		triangleData.bitangent[i] = texelFetch(vertex_bitangents, vertexIndex).rgb;
		triangleData.matId = uint(texelFetch(vertex_matId, vertexIndex).r);
	}
}

Ray calculatePixelRay()
{
	vec4 worldSpacePositionNear = ubo.data.camPos;

	vec4 clipSpacePositionFar = vec4(inUV * 2.0 - 1.0f, 1.0, 1.0);
	vec4 viewSpacePositionFar = ubo.data.inverseProjection* clipSpacePositionFar;
	viewSpacePositionFar /= viewSpacePositionFar.w;
	vec4 worldSpacePositionFar = ubo.data.inverseView * viewSpacePositionFar;

	//outColor.rgb = clipSpacePositionNear.rgb;

	Ray ray;
	ray.origin = worldSpacePositionNear.xyz;
	ray.direction = normalize((worldSpacePositionFar-worldSpacePositionNear).xyz);

	return ray;
	//return normalize((viewSpacePositionFar-viewSpacePositionNear).xyz);
}


// https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
void barycentric(vec3 p, vec3 a, vec3 b, vec3 c, out float u, out float v, out float w)
{
    vec3 v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    v = (d11 * d20 - d01 * d21) / denom;
    w = (d00 * d21 - d01 * d20) / denom;
    u = 1.0f - v - w;
}



void interpolateTriangleData(TriangleData triangleData, out InterpolatedTriangleData interpolatedTriangleData, Ray ray)
{
	vec3 A = (ubo.data.model*vec4(triangleData.pos[0], 1.0)).xyz;
	vec3 B = (ubo.data.model*vec4(triangleData.pos[1], 1.0)).xyz;
	vec3 C = (ubo.data.model*vec4(triangleData.pos[2], 1.0)).xyz;
	vec3 AB = B-A;
	vec3 AC = C-A;
	vec3 BC = C-B;
	vec3 N = cross(AB,AC);
	float d = dot(N,(A - ray.origin))/dot(N,ray.direction);
	vec3 P = d*ray.direction + ray.origin;
	float alpha, beta, gamma;
	barycentric(P, A, B, C, alpha, beta, gamma);


	interpolatedTriangleData.pos = alpha * triangleData.pos[0] + beta * triangleData.pos[1] + gamma * triangleData.pos[2];
	interpolatedTriangleData.normal = alpha * triangleData.normal[0] + beta * triangleData.normal[1] + gamma * triangleData.normal[2];
	interpolatedTriangleData.texCoords = alpha * triangleData.texCoords[0] + beta * triangleData.texCoords[1] + gamma * triangleData.texCoords[2];
	interpolatedTriangleData.tangent = alpha * triangleData.tangent[0] + beta * triangleData.tangent[1] + gamma * triangleData.tangent[2];
	interpolatedTriangleData.bitangent = alpha * triangleData.bitangent[0] + beta * triangleData.bitangent[1] + gamma * triangleData.bitangent[2];
	interpolatedTriangleData.matId = triangleData.matId;

	outColor.rgb = vec3(0.5+0.5*interpolatedTriangleData.texCoords.rg,0.0);
}
//ubo.data.camPos.xyz
void calculateShadingVectors(inout ShadingData data, vec3 lightPosition, vec3 viewPosition)
{
	data.view			= normalize(viewPosition - data.worldPos);
	data.lightVector	=  normalize(lightPosition - data.worldPos);
	data.halfWayVector	= normalize(data.lightVector + data.view);
	data.dotLH			= clamp(dot(data.lightVector, data.halfWayVector), 0.0, 1.0);
	data.dotNV			= abs(dot(data.normal, data.view)+ 0.00001);
	data.dotNH			= clamp(dot(data.normal, data.halfWayVector), 0.0, 1.0);
	data.dotNL			= clamp(dot(data.normal, data.lightVector), 0.0, 1.0);
}

ShadingData prepareShadingData(InterpolatedTriangleData interpolatedTriangleData, vec3 viewPosition)
{
	vec2 uvCoord = interpolatedTriangleData.texCoords;
	vec3 N = normalize(interpolatedTriangleData.normal);
	vec3 T = normalize(interpolatedTriangleData.tangent);
	T = normalize(T - dot(T, N) * N);
	vec3 B = -normalize(cross(N, T));
	uint idx = interpolatedTriangleData.matId;
	ShadingData data;
	data.TNB = mat3(T,B,N);
	data.normal			= normalize(data.TNB*(2.0 * (texture(tex[mat.matData[idx].idx_map_normal], uvCoord).rgb) - 1.0));
	data.surfaceNormal = N;
	data.diffuse_albedo  = texture(tex[mat.matData[idx].idx_map_diffuse], uvCoord).rgb;
	vec3 specularData	= texture(tex[mat.matData[idx].idx_map_specular], uvCoord).rgb;
	data.metalicity = specularData.b;
	data.specular_color = vec3(1.0);//texture(tex[mat.matData[idx].idx_map_specular], vs_out.uvCoord).rgb;
	data.fresnel_0		= mix(vec3(0.02f), data.diffuse_albedo, data.metalicity); //ToDo get value
	data.diffuse_color  = (1.0 - data.metalicity) * data.diffuse_albedo;
	if(mat.matData[idx].idx_map_emissive != MAX_UINT) 
	{
		data.emissive =  texture(tex[mat.matData[idx].idx_map_emissive], uvCoord).rgb;
	}
	else
	{
		data.emissive = vec3(0.0);
	}
	data.roughness		= specularData.g;
	data.PI				= 3.141592f;

	data.worldPos = (ubo.data.model * vec4(interpolatedTriangleData.pos,1.0)).xyz;
	data.modelPos = interpolatedTriangleData.pos.xyz;;

	//outColor.rgb = data.modelPos;

	calculateShadingVectors(data, ubo.data.lightPosition.xyz, viewPosition);
	return data;
}


ShadingData prepareShadingData(InterpolatedTriangleData interpolatedTriangleData, float bias, vec3 viewPosition)
{
	vec2 uvCoord = interpolatedTriangleData.texCoords;
	vec3 N = normalize(interpolatedTriangleData.normal);
	vec3 T = normalize(interpolatedTriangleData.tangent);
	T = normalize(T - dot(T, N) * N);
	vec3 B = -normalize(cross(N, T));
	uint idx = interpolatedTriangleData.matId;
	ShadingData data;
	data.TNB = mat3(T,B,N);
	data.normal			= normalize(data.TNB*(2.0 * (texture(tex[mat.matData[idx].idx_map_normal], uvCoord).rgb) - 1.0));
	data.surfaceNormal = N;
	data.diffuse_albedo = texture(tex[mat.matData[idx].idx_map_diffuse], uvCoord, bias).rgb;
	vec3 specularData	= texture(tex[mat.matData[idx].idx_map_specular], uvCoord, bias).rgb;
	data.metalicity = specularData.b;
	data.specular_color = vec3(1.0);//texture(tex[mat.matData[idx].idx_map_specular], vs_out.uvCoord).rgb;
	data.fresnel_0		= mix(vec3(0.02f), data.diffuse_albedo, data.metalicity); //ToDo get value
	data.diffuse_color  = (1.0 - data.metalicity) * data.diffuse_albedo;
	data.roughness		= specularData.g;
	if(mat.matData[idx].idx_map_emissive != MAX_UINT)
	{
		data.emissive =  texture(tex[mat.matData[idx].idx_map_emissive], uvCoord, bias).rgb;
	}
	else
	{
		data.emissive = vec3(0.0);
	}
	data.PI				= 3.141592f;
	data.worldPos = (ubo.data.model * vec4(interpolatedTriangleData.pos,1.0)).xyz;
	data.modelPos = interpolatedTriangleData.pos.xyz;;
	calculateShadingVectors(data, ubo.data.lightPosition.xyz, viewPosition);
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
	blendingData.color.rgb					+= color.rgb * color.a * weight;
	blendingData.color.a					*= 1.0-color.a;
	blendingData.weight_color				+= color.a * weight;
	// Normal Blending
	blendingData.normal.rgb					+= normal.rgb * weight * normal.a;
	blendingData.normal.a					*= 1.0-normal.a;
	blendingData.weight_normal				+= weight * normal.a;
	// Roughness & Metalicity
	blendingData.specularData.rgb			+= specularData.rgb * weight * specularData.a;
	blendingData.specularData.a				*= 1.0-specularData.a;
	blendingData.weight_specularData		+= specularData.a * weight;
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
		vec3 N = data.surfaceNormal;
		vec3 T = d.inverseTransposedTransformation[0].xyz;
		T = -normalize(T-N*dot(N,T));
		vec3 B = -normalize(cross(N, T));
		mat3 TBN = mat3(T,B,N);
		decalNormal.xyz		= normalize(((TBN * decalNormal.xyz)).rgb);
		//decalNormal.xyz		= normalize(((data.TNB * decalNormal.xyz)).rgb);
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




bool applyDecalCursor(inout DecalBlendingData blendingData[BLENDING_CHANNELS], ShadingData data, vec3 dx_world, vec3 dy_world)
{
	// Get Position in decal coordinate system
	DecalData d = cursor.decal;
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
		vec3 N = data.surfaceNormal;
		vec3 T = d.inverseTransposedTransformation[0].xyz;
		T = -normalize(T-N*dot(N,T));
		vec3 B = -normalize(cross(N, T));
		mat3 TBN = mat3(T,B,N);
		decalNormal.xyz		= normalize(((TBN * decalNormal.xyz)).rgb);
		//decalNormal.xyz		= normalize(((data.TNB * decalNormal.xyz)).rgb);
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

	vec3 dx_vtc     = dFdx(data.worldPos.xyz);
    vec3 dy_vtc     = dFdy(data.worldPos.xyz);

	vec3 origin = data.worldPos;
	vec3 direction = vec3(1.0, 0.0, 0.0);
	float tMin = 0.0;
	float tMax = 0.0;
	bool query = false;
	bool hit;

	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, topLevelAS,
	gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsCullBackFacingTrianglesEXT | gl_RayFlagsCullFrontFacingTrianglesEXT,
	0xFF, origin, tMin, direction, tMax);


	while(rayQueryProceedEXT(rayQuery))
	{
		applySingleDecal(blendingData, rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false), data, dx_vtc, dy_vtc);
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

	APPLY_DECAL_EDITOR

	blending_color.rgb =		blendingData[0].color.rgb		/	blendingData[0].weight_color;
	blending_normal.rgb =		blendingData[0].normal.rgb		/	blendingData[0].weight_normal;
	blending_specularData.rgb = blendingData[0].specularData.rgb/	blendingData[0].weight_specularData;

	blending_color.a			= 1.0-blendingData[0].color.a;
	blending_normal.a			= 1.0-blendingData[0].normal.a;
	blending_specularData.a		= 1.0-blendingData[0].specularData.a;

	//blending_color.rgb = vec3(0.1+blendingData[0].color.a);
	//blending_color.rgb = vec3(1.0,0.0,0.0);
	//blending_color.a = 1.0;

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

	blending_color =		blendingData[3].color		/	blendingData[3].weight_color;
	blending_normal =		blendingData[3].normal		/	blendingData[3].weight_normal;
	blending_specularData = blendingData[3].specularData/	blendingData[3].weight_specularData;
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
	rayQueryInitializeEXT(rayQuery, topLevelASPointLights, gl_RayFlagsOpaqueEXT, 0xFF, origin, tMin, direction, tMax);
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
	//outColor.rgb = outColor.rgb +  (boxColor)*(boxCount);
	
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

vec3 applySinglePointLight(ShadingData data, PointLightData pointLight, vec3 viewPosition, float coefficiant)
{
	vec3 lightPosition = pointLight.position.xyz;//+ vec3(0.0,0.0,0.2);
	calculateShadingVectors(data, lightPosition, viewPosition);
	vec3 brdf = evaluateBRDF(data);
	float lightIntensity = calculateLightIntesity(length(lightPosition - data.worldPos.xyz))* pointLight.color.a;
	return coefficiant*brdf*data.dotNL*lightIntensity * pointLight.color.rgb;
}

void applyPointLights(inout vec4 outColor, ShadingData data, vec3 viewPosition, float coefficiant)
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
			outColor.rgb += applySinglePointLight(data, pointLights.data[rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false)], viewPosition, coefficiant);
		}
	} else {
		for(int i = 0; i<POINT_LIGHTS;i++)
		{
			outColor.rgb += applySinglePointLight(data, pointLights.data[i], viewPosition, coefficiant);
		}
	}
}

void applyPrimaryPointLight(inout vec4 outColor, ShadingData data, vec3 viewPosition, float coefficiant)
{
	vec3 lightPosition = ubo.data.lightPosition.xyz;
	calculateShadingVectors(data, lightPosition, viewPosition);
	vec3 brdf = evaluateBRDF(data);
	float lightIntensity = calculateLightIntesity(length(ubo.data.lightPosition.xyz - data.worldPos.xyz)) * ubo.data.lightColor.a;
	outColor.rgb += coefficiant*brdf*data.dotNL*lightIntensity * ubo.data.lightColor.rgb;
}

void applyEditorPointLight(inout vec4 outColor, ShadingData data, vec3 viewPosition, float coefficiant)
{
	vec3 lightPosition = cursor.light.position.xyz;
	calculateShadingVectors(data, lightPosition, viewPosition);
	vec3 brdf = evaluateBRDF(data);
	float lightIntensity = calculateLightIntesity(length(lightPosition - data.worldPos.xyz)) * cursor.light.color.a;
	outColor.rgb += coefficiant*brdf*data.dotNL*lightIntensity * cursor.light.color.rgb;
}


void applyReflection(inout vec4 outColor, ShadingData data)
{

	float bias = 8.0;
	calculateShadingVectors(data, ubo.data.lightPosition.xyz, ubo.data.camPos.xyz);
	Ray ray;
	ray.origin = data.worldPos;
	ray.direction = -reflect(ubo.data.camPos.xyz-data.worldPos, data.normal);
	float tMin = 0.1;
	float tMax = INFINITY;
	float coefficiant = 1.0 - data.roughness/MAX_REFLECTION_ROUGHNESS;
	outColor.rgb *= 1.0 - coefficiant;

	for(int i = 0; i < MAX_RECURSION; i++)
	{
		rayQueryEXT rayQuery;
		rayQueryInitializeEXT(rayQuery, topLevelASScene, gl_RayFlagsOpaqueEXT | gl_RayFlagsCullBackFacingTrianglesEXT , 0xFF, ray.origin, tMin, ray.direction, tMax);
		while(rayQueryProceedEXT(rayQuery))
		{
			rayQueryConfirmIntersectionEXT(rayQuery);
		}
		bool hit = (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT);
		if(hit)
		{
			
			int triangleId = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
			TriangleData triangleData;
			InterpolatedTriangleData interpolatedTriangleData;
			getTriangleData(triangleId, triangleData);
			interpolateTriangleData(triangleData, interpolatedTriangleData, ray);
			ShadingData data = prepareShadingData(interpolatedTriangleData, bias, ray.origin);
			vec3 viewPosition = ray.origin;
			APPLY_RAY_QUERY_DECALS

			if((data.roughness<MAX_REFLECTION_ROUGHNESS||data.metalicity>0.5)&&i<MAX_RECURSION-1)
			{
				ray.origin = data.worldPos;
				ray.direction = reflect(ray.direction, data.normal);
			}
			else
			{
				APPLY_PRIMARY_POINT_LIGHT
				APPLY_LIGHT_EDITOR
				APPLY_RAY_QUERY_SHADOWS
				APPLY_SCENE_POINT_LIGHTS
				APPLY_AMBIENT_TERM
				APPLY_EMISSIVE_TERM
				break;
			}
		}
	}
}


void main()
{
	uint triangleId = subpassLoad(samplerId).r;
	if(triangleId == 0)
	{
		discard;
	}
	TriangleData triangleData;
	InterpolatedTriangleData interpolatedTriangleData;
	Ray ray = calculatePixelRay();
	getTriangleData(triangleId, triangleData);
	interpolateTriangleData(triangleData, interpolatedTriangleData, ray);
	ShadingData data = prepareShadingData(interpolatedTriangleData, ubo.data.camPos.xyz);
	outColor = vec4(0.0, 0.0, 0.0, 1.0);
	vec3 viewPosition = ubo.data.camPos.xyz;
	float coefficiant = 1.0;

	APPLY_RAY_QUERY_DECALS
	APPLY_PRIMARY_POINT_LIGHT
	APPLY_LIGHT_EDITOR
	APPLY_RAY_QUERY_SHADOWS
	APPLY_SCENE_POINT_LIGHTS
	APPLY_AMBIENT_TERM
	APPLY_EMISSIVE_TERM
	APPLY_RAY_QUERY_DECAL_AABBS

	APPLY_RAY_QUERY_REFLECTIONS

	convertLinearToSRGB(outColor);

	//outColor.rgb = vec3(0.1*(triangleId%10), 0.1*((triangleId/10)%10),  0.1*((triangleId/100)%10));

}


