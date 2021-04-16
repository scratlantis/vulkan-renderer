#pragma once

#include "Vertex.h"

struct Mesh
{
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

};