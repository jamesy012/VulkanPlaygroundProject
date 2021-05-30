#pragma once

struct VertexType {
   VkVertexInputBindingDescription mBindingDescription;
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