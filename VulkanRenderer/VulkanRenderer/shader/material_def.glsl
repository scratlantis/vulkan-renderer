
struct MaterialData
{
	//vec4 ambient;
	//vec4 diffuse;
	//vec4 specular;

	uint slotUsed;
	uint idx_map_specular;
	uint idx_map_diffuse;
	uint idx_map_normal;

	uint idx_map_alpha;
	uint idx_map_emissive;
	uint placeholder2;
	uint placeholder3;
};

