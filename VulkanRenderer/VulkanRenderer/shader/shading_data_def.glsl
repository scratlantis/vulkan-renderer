struct ShadingData
{
	vec3	view;				//V
	vec3	normal;				//N
	vec3	halfWayVector;		//H
	vec3	lightVector;		//L

	vec3	modelPos;
	vec3	worldPos;

	vec3	specular_color;
	vec3	diffuse_color;

	
	vec3	fresnel_0;			// Color of specular reflection at 0 degree
	float	roughness;

	float	dotNV;
	float	dotLH;
	float	dotNH;
	float	dotNL;
	
	float	PI;
	float	metalicity;
	vec3	diffuse_albedo;
	vec3	emissive;

	vec3	surfaceNormal;
	mat3 TNB;
} data;