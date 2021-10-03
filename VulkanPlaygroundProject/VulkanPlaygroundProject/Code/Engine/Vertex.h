#pragma once

#include <glm/glm.hpp>

struct VertexType {
	std::vector<VkVertexInputBindingDescription> mBindingDescription;
	std::vector<VkVertexInputAttributeDescription> mAttributeDescriptions;
};

extern VertexType VertexTypeSimple;
struct VertexSimple {
	glm::vec2 pos;
};

extern VertexType VertexTypeDefault;
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec4 color;
	glm::vec2 texCoord;
	glm::vec3 tangent;
	glm::vec4 jointIndex;
	glm::vec4 jointWeight;
};

extern VertexType VertexTypeInstanced;
struct VertexInstanceVertex {
	glm::vec3 vertexPos;
	glm::vec3 vertexNormal;
	glm::vec4 vertexColor;
	glm::vec2 vertexTexCoord;
};
struct VertexInstanceInstance {
	glm::vec3 instancePos;
	//glm::mat4 instanceMatrix;
};

#include "Buffer.h"

enum class VertexPrimitives { QUAD };

class BufferVertex : public Buffer {
public:
	bool Create( VkDeviceSize aSize ) override {
		return Buffer::Create( aSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY );
	}

	bool CreatePrimitive( VertexPrimitives aPrim ) {
		ASSERT_IF( aPrim == VertexPrimitives::QUAD );
		bool result = Create( sizeof( Vertex ) * 6 );
		if ( result == false ) {
			return result;
		}
		Vertex verts[6]{};
		verts[0].pos = glm::vec3( -1.0f, -1.0f, 0 );
		verts[1].pos = glm::vec3( 1.0f, -1.0f, 0 );
		verts[2].pos = glm::vec3( -1.0f, 1.0f, 0 );
		verts[3].pos = glm::vec3( 1.0f, -1.0f, 0 );
		verts[4].pos = glm::vec3( 1.0f, 1.0f, 0 );
		verts[5].pos = glm::vec3( -1.0f, 1.0f, 0 );
		BufferStaging staging;
		staging.Create( GetSize() );
		{
			void* data;
			staging.Map( &data );
			memcpy( data, verts, sizeof( Vertex ) * 6 );
			staging.UnMap();
		}
		CopyFrom( &staging );
		staging.Destroy();
		return false;
	}

	void Bind( VkCommandBuffer aBuffer, uint32_t aBindingIndex = 0 ) {
		VkBuffer vertexBuffers[] = { GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers( aBuffer, aBindingIndex, 1, vertexBuffers, offsets );
	}
};