
struct UBO
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
};