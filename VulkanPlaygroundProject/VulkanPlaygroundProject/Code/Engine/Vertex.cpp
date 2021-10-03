#include "stdafx.h"
#include "Vertex.h"

VertexType VertexTypeSimple = {
	{
		{0, sizeof(VertexSimple), VK_VERTEX_INPUT_RATE_VERTEX}
	},
	{
		{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexSimple, pos)}
	}
};

VertexType VertexTypeDefault = {
	{
		{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
	},
	{
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
		{2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)},
		{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)},
		{4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
		{5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, jointIndex)},
		{6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, jointWeight)}
	}
};

VertexType VertexTypeInstanced = {
	{
		{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX},
		{1, sizeof(VertexInstanceInstance), VK_VERTEX_INPUT_RATE_INSTANCE}
	},
	{
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
		{2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)},
		{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)},
		{4, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexInstanceInstance, instancePos)}
	 }
};