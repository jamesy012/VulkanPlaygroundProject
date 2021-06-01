#include "stdafx.h"
#include "Model.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <assimp/pbrmaterial.h>     // PBR

static inline glm::vec4 vec4_cast(const aiColor4D& v) { return glm::vec4(v.r, v.g, v.b, v.a); }
static inline glm::vec3 vec3_cast(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
static inline glm::vec2 vec2_cast(const aiVector3D& v) { return glm::vec2(v.x, v.y); }
static inline glm::quat quat_cast(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }
static inline glm::mat4 mat4_cast(const aiMatrix4x4& m) { return glm::transpose(glm::make_mat4(&m.a1)); }
static inline glm::mat4 mat4_cast(const aiMatrix3x3& m) { return glm::transpose(glm::make_mat3(&m.a1)); }

bool Model::LoadModel(std::string aPath) {
   Assimp::Importer importer;
   const aiScene* scene = importer.ReadFile(aPath,
                             aiProcess_CalcTangentSpace |
                             aiProcess_Triangulate |
                             aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
                             aiProcess_SortByPType);

   if (scene == nullptr) {
      ASSERT_RET_FALSE("Failed to load");
   }
   {
      size_t index = aPath.find_last_of('/') + 1;
      if (index == 0) {
         index = aPath.find_last_of('\\') + 1;
      }
      mName = aPath.substr(index);
      mPath = aPath.substr(0, index);
      index = mName.find_last_of('.');
      mName = mName.substr(0, index);
   }

   ProcessMeshs(scene);
   return true;
}

void Model::ProcessMeshs(const aiScene* aScene) {
   mMeshs.resize(aScene->mNumMeshes);
   mBase = new Node();
   ProcessMesh(aScene, aScene->mRootNode, mBase);

   mVertexBuffer.Create(mVertices.size() * sizeof(Vertex));
   mIndexBuffer.Create(mIndices.size() * sizeof(uint32_t));
   BufferStaging staging;
   staging.Create(std::max(mVertexBuffer.GetAllocatedSize(), mIndexBuffer.GetAllocatedSize()));
   void* data;
   staging.Map(&data);
   memcpy(data, mVertices.data(), mVertexBuffer.GetSize());
   mVertexBuffer.CopyFrom(&staging);
   memcpy(data, mIndices.data(), mIndexBuffer.GetSize());
   mIndexBuffer.CopyFrom(&staging);
   staging.UnMap();
   staging.Destroy();
}

void Model::ProcessMesh(const aiScene* aScene, const aiNode* aNode, Node* aParent) {
   Node* node = new Node();
   mNodes.push_back(node);

   //parent and transform
   if (aParent) {
      aParent->mChildren.push_back(node);
      node->mParent = aParent;
   }
   node->mName = aNode->mName.C_Str();

   node->mRenderMatrix = mat4_cast(aNode->mTransformation);

   //meshs
   node->mMesh.resize(aNode->mNumMeshes);
   for (unsigned int i = 0; i < aNode->mNumMeshes; i++) {
      Mesh& mesh = node->mMesh[i];
      unsigned int meshIndex = aNode->mMeshes[i];
      aiMesh* assimpMesh = aScene->mMeshes[meshIndex];
      mMeshs[meshIndex] = &node->mMesh[i];

      //material
      mesh.mMaterialID = assimpMesh->mMaterialIndex;
      mesh.mStartIndex = static_cast<int>(mIndices.size());
      mesh.mStartVertex = static_cast<int>(mVertices.size());
      mesh.mCount = assimpMesh->mNumFaces * 3;//always triangle

      //verts
      for (unsigned int i = 0; i < assimpMesh->mNumVertices; i++) {
         Vertex vert;
         vert.pos = vec3_cast(assimpMesh->mVertices[i]);
         if (assimpMesh->HasNormals()) {
            vert.normal = vec3_cast(assimpMesh->mNormals[i]);
         } else {
            vert.normal = glm::vec3(0);
         }
         if (assimpMesh->GetNumUVChannels() >= 1) {
            vert.texCoord = vec2_cast(assimpMesh->mTextureCoords[0][i]);
         } else {
            vert.texCoord = glm::vec2(0);
         }
         if (assimpMesh->GetNumColorChannels() >= 1) {
            vert.color = vec4_cast(assimpMesh->mColors[0][i]);
         } else {
            vert.color = glm::vec4(1);
         }
         if (assimpMesh->HasTangentsAndBitangents()) {
            vert.tangent = vec3_cast(assimpMesh->mTangents[i]);
         } else {
            vert.tangent = glm::vec3(0);
         }
         vert.jointIndex = glm::vec4(-1);
         vert.jointWeight = glm::vec4(-1);
         mVertices.push_back(vert);
      }

      //indices
      for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++) {
         assert(assimpMesh->mFaces[i].mNumIndices == 3);
         for (unsigned int q = 0; q < assimpMesh->mFaces[i].mNumIndices; q++) {
            mIndices.push_back(mesh.mStartVertex + assimpMesh->mFaces[i].mIndices[q]);
         }
      }
   }

   for (unsigned int i = 0; i < aNode->mNumChildren; i++) {
      ProcessMesh(aScene, aNode->mChildren[i], node);
   }
}

void Model::Render(VkCommandBuffer aCommandBuffer, VkPipelineLayout aPipelineLayout, RenderMode aRenderMode) {
   assert(aRenderMode != RenderMode::ALL);
   assert((aRenderMode & (aRenderMode-1)) == 0);
   if (!(mRenderModes & aRenderMode)) {
      return;
   }
   mVertexBuffer.Bind(aCommandBuffer);
   mIndexBuffer.Bind(aCommandBuffer);

   Render(aCommandBuffer, aPipelineLayout, aRenderMode, mBase, glm::identity<glm::mat4>());

}

void Model::Destroy() {
   mVertexBuffer.Destroy();
   mIndexBuffer.Destroy();
   mVertices.clear();
   mIndices.clear();
}

void Model::Render(VkCommandBuffer aCommandBuffer, VkPipelineLayout aPipelineLayout, RenderMode aRenderMode, Node* aNode, glm::mat4 aMatrix) {
   if (!aNode->mMesh.empty()) {

      for (int i = 0; i < aNode->mMesh.size(); i++) {
         vkCmdDrawIndexed(aCommandBuffer, static_cast<uint32_t>(aNode->mMesh[i].mCount), 1, aNode->mMesh[i].mStartIndex, 0, 0);
      }
   }

   for (int i = 0; i < aNode->mChildren.size(); i++) {
      Render(aCommandBuffer, aPipelineLayout, aRenderMode, aNode->mChildren[i], glm::identity<glm::mat4>());
   }
}
