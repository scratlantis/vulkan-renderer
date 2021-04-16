#pragma once


#include "MaterialLoader.h"
#include "DecalLoader.h"
#include "PointLightLoader.h"

#define GENERATE_DECALS false
#define GENERATE_LIGHTS false

class MeshLoader
{
private:
	MaterialLoader* m_materialLoader;
	DecalLoader* m_decalLoader;
	PointLightLoader* m_pointLightLoader;
public:
	MeshLoader(MaterialLoader* materialLoader, DecalLoader* decalLoader, PointLightLoader* pointLightLoader);

	MeshLoader();

	void load_Decals(const char* textureBaseDir);

	void createMesh(const char* path, const char* mtlBaseDir, const char* textureBaseDir, Node& node);

	void setMaterialLoader(MaterialLoader* materialLoader);

	void setDecalLoader(DecalLoader* decalLoader);

	void setPointLightLoader(PointLightLoader* pointLightLoader);
};