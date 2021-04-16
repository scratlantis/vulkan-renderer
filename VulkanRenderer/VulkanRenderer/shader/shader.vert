#version 450
//#extension GL_ARB_seperate_objects : enable
//#extension GL_KHR_vulkan_glsl : enable

#define EPSILON  0.0001
#define LIGHT_INTENSITY 8

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 inUVCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;
layout(location = 6) in uint materialIndex;



layout(location = 1) out VS_INTERPOLATET
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

layout(location = 0) flat out uint fragMaterialIndex;





layout(binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 projection;
	mat4 inverseModel;
	mat4 inverseView;
	mat4 inverseProjection;
	vec4 lightPosition;
	vec4 lightPositionModel;
	vec4 lightColor;
	vec4 camPos;
	vec4 camPosModel;
	uint counter;
	uint decalCount;
	uint width;
	uint height;
} ubo;





void main()
{
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(pos, 1.0);
	vs_out.worldPos = ubo.model * vec4(pos, 1.0);
	vs_out.modelPos = vec4(pos, 1.0);

	//mat3 mv = mat3(ubo.view)*mat3(ubo.model);

	vs_out.uvCoord = vec2(inUVCoord.x,1.0-inUVCoord.y); //Somehow textures are inverted on y
	fragMaterialIndex = materialIndex;




	vs_out.fragNormal =  normalize(vec3(ubo.model * vec4(inNormal, 0)));
	vs_out.fragTangent = normalize(vec3(ubo.model * vec4(inTangent, 0)));
	vs_out.fragBitangent = normalize(vec3(ubo.model * vec4(inBitangent, 0)));


	//mat3 TBN = transpose(mat3(vs_out.fragTangent, vs_out.fragBitangent, vs_out.fragNormal));

	vs_out.lightVector = normalize(ubo.lightPosition.xyz - vs_out.worldPos.xyz);
	vs_out.viewVector = normalize(ubo.camPos.xyz - vs_out.worldPos.xyz);
	

}


