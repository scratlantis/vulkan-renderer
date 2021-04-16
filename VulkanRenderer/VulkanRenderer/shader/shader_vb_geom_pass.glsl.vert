#version 460
#extension GL_GOOGLE_include_directive : enable
//#extension GL_ARB_seperate_objects : enable
//#extension GL_KHR_vulkan_glsl : enable

#define EPSILON  0.0001
#define LIGHT_INTENSITY 8
#include "ubo_def.glsl"
out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) in vec3 pos;



layout(location = 0) flat out uint primitiveIndex;


layout(binding = 0) uniform UBO_DATA
{
	UBO data;
} ubo;





void main()
{
	gl_Position = ubo.data.projection * ubo.data.view * ubo.data.model * vec4(pos, 1.0);
	//uvCoord = vec2(inUVCoord.x,1.0-inUVCoord.y); //Somehow textures are inverted on y
	primitiveIndex = gl_VertexIndex / 3;
}