struct DecalData
{
	mat4 inverseTransposedTransformation;
    mat4 inverseTransformation;

    vec4 uvCoordAB;
    vec4 uvCoordCD;

	uint materialIndex;
	float weight;
	uint channel;
	uint placeholder3;
};

struct DecalBlendingData
{
	vec4 color;
	vec4 normal;
	vec4 specularData;

	float weight_color;
	float weight_normal;
	float weight_specularData;
};

