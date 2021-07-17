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

bool Model::LoadModel(std::string aPath, VkDescriptorSetLayout aMaterialDescriptorSet, std::vector<VkWriteDescriptorSet> aWriteSets) {
   LOG_SCOPED_NAME("Model Loading");
   LOG("%s\n", aPath.c_str());
   Assimp::Importer importer;
   const aiScene* scene = importer.ReadFile(aPath,
                                            aiProcess_CalcTangentSpace |
                                            aiProcess_Triangulate |
                                            aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
                                            aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph |
                                            aiProcess_GenBoundingBoxes);

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
   LOG("Starting Mesh\n");
   ProcessMeshs(scene);
   LOG("Starting Materials\n");
   ProcessMaterials(scene);
   LOG("Starting Images\n");
   LoadImages();

   {
      for (size_t i = 0; i < mMaterials.size(); i++) {
         VkDescriptorSetAllocateInfo setAllocate{};
         setAllocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
         setAllocate.descriptorPool = _VulkanManager->GetDescriptorPool();
         setAllocate.descriptorSetCount = 1;
         setAllocate.pSetLayouts = &aMaterialDescriptorSet;
         vkAllocateDescriptorSets(_VulkanManager->GetDevice(), &setAllocate, &mMaterials[i].mDescriptorSet);
         DebugSetObjName(VK_OBJECT_TYPE_DESCRIPTOR_SET, mMaterials[i].mDescriptorSet, mName + " Material set " + std::to_string(i));

         std::vector<VkWriteDescriptorSet> write = aWriteSets;
         VkDescriptorImageInfo info;
         if (!mMaterials[i].mDiffuse.empty()) {
            write.push_back(GetWriteDescriptorSet(info, mMaterials[i].mDiffuse[0], mMaterials[i].mDescriptorSet, _VulkanManager->GetDefaultSampler(), 0));
         } else {
            write.push_back(GetWriteDescriptorSet(info, &mImages[0], mMaterials[i].mDescriptorSet, _VulkanManager->GetDefaultSampler(), 0));
         }
         for (size_t q = 0; q < write.size(); q++) {
            write[q].dstSet = mMaterials[i].mDescriptorSet;
         }
         vkUpdateDescriptorSets(_VulkanManager->GetDevice(), static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);
      }

   }

   LOG("Done\n");
   return true;
}

void Model::ProcessMeshs(const aiScene* aScene) {
   mMeshs.resize(aScene->mNumMeshes);
   ProcessMesh(aScene, aScene->mRootNode, &mBase);

   mVertexBuffer.Create(mVertices.size() * sizeof(Vertex));
   mIndexBuffer.Create(mIndices.size() * sizeof(uint32_t));
   mVertexBuffer.SetName(mName + " Vertex Buffer");
   mIndexBuffer.SetName(mName + " Index Buffer");

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
      node->mTransform.SetParent(&node->mParent->mTransform);
   }
   node->mName = aNode->mName.C_Str();

   node->mTransform.SetMatrix(mat4_cast(aNode->mTransformation));

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
         ASSERT_IF(assimpMesh->mFaces[i].mNumIndices == 3);
         for (unsigned int q = 0; q < assimpMesh->mFaces[i].mNumIndices; q++) {
            mIndices.push_back(mesh.mStartVertex + assimpMesh->mFaces[i].mIndices[q]);
         }
      }

      mesh.mMin = vec3_cast(assimpMesh->mAABB.mMin);
      mesh.mMax = vec3_cast(assimpMesh->mAABB.mMax);
   }

   for (unsigned int i = 0; i < aNode->mNumChildren; i++) {
      ProcessMesh(aScene, aNode->mChildren[i], node);
   }
}

void Model::ProcessMaterials(const aiScene* aScene) {
   mMaterials.resize(aScene->mNumMaterials);

   for (unsigned int matIdx = 0; matIdx < aScene->mNumMaterials; matIdx++) {
      aiMaterial* assimpMaterial = aScene->mMaterials[matIdx];
      Material* material = &mMaterials[matIdx];
      const int numTypes = 2;
      const aiTextureType assimpTypes[numTypes] = { aiTextureType_DIFFUSE, aiTextureType_NORMALS };
      for (int imgType = 0; imgType < numTypes; imgType++) {
         const int numTextures = assimpMaterial->GetTextureCount(assimpTypes[imgType]);

         for (int imgIdx = 0; imgIdx < numTextures; imgIdx++) {
            aiString path;
            //These cause a memory fault when used??
            //aiTextureMapping mapping;
            //unsigned int uvIndex;
            //ai_real blend;
            //aiTextureOp op;
            //aiTextureMapMode mapMode;

            //if (assimpMaterial->GetTexture(assimpTypes[i], imgIdx, &path, &mapping, &uvIndex, &blend, &op, &mapMode) == AI_FAILURE) {
            if (assimpMaterial->GetTexture(assimpTypes[imgType], imgIdx, &path) == AI_FAILURE) {
               continue;
            }

            std::string filePath = path.C_Str();
            ImageLoader* imgLoader = nullptr;
            for (size_t i = 0; i < mImageLoader.size(); i++) {
               if (mImageLoader[i].mPath.compare(filePath) == 0) {
                  imgLoader = &mImageLoader[i];
                  break;
               }
            }

            //we have already seen this image
            if (imgLoader == nullptr) {
               ImageLoader loader;
               loader.mPath = filePath;
               loader.mType = ImageType(imgType);
               mImageLoader.push_back(loader);
               imgLoader = &mImageLoader[mImageLoader.size() - 1];
            }
            imgLoader->mMaterialRef.push_back(material);
         }
      }
   }
}

void Model::LoadImages() {
   mImages.resize(mImageLoader.size());
   for (size_t i = 0; i < mImageLoader.size(); i++) {
      const ImageLoader& imgLoader = mImageLoader[i];
      Image* img = &mImages[i];

      img->LoadImage(mPath + imgLoader.mPath);

      for (size_t materials = 0; materials < imgLoader.mMaterialRef.size(); materials++) {
         switch (imgLoader.mType) {
            case ImageType::DIFFUSE:
               imgLoader.mMaterialRef[materials]->mDiffuse.push_back(img);
               break;
            case ImageType::NORMAL:
               imgLoader.mMaterialRef[materials]->mNormal.push_back(img);
               break;
            default:
               ASSERT(false);
         }
      }

   }
}

void Model::Render(DescriptorUBO* aRenderDescriptor, RenderMode aRenderMode) {
   ASSERT_IF((aRenderMode & (aRenderMode - 1)) == 0);
   if (!(mRenderModes & aRenderMode)) {
      return;
   }
   PROFILE_START_SCOPED("Model Render: " + mName);
   _VulkanManager->DebugMarkerStart(aRenderDescriptor->mCommandBuffer, mName);
   mVertexBuffer.Bind(aRenderDescriptor->mCommandBuffer);
   mIndexBuffer.Bind(aRenderDescriptor->mCommandBuffer);

   for (size_t i = 0; i < mNodes.size(); i++) {
      Node* node = mNodes[i];
      if (!node->mMesh.empty()) {
         ObjectUBO ubo;
         ubo.mModel = node->GetMatrixWithParents();
         aRenderDescriptor->UpdateObjectAndBind(&ubo);

         for (int i = 0; i < node->mMesh.size(); i++) {
            Mesh& mesh = node->mMesh[i];
#if defined(CULLING_TEST)
            glm::vec3 min = glm::vec4(mesh.mMin, 0) * node->mTransform.GetGlobalMatrix();
            glm::vec3 max = glm::vec4(mesh.mMax, 0) * node->mTransform.GetGlobalMatrix();
            if ((pos.x > min.x && pos.x < max.x) && (pos.y > min.y && pos.y < max.y) && (pos.z > min.z && pos.z < max.z))
#endif
            {
               if (aRenderMode == RenderMode::NORMAL) {
                  Material& material = mMaterials[node->mMesh[i].mMaterialID];
                  if (!material.mDiffuse.empty()) {
                     vkCmdBindDescriptorSets(aRenderDescriptor->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aRenderDescriptor->mPipelineLayout, 2, 1, &material.mDescriptorSet, 0, nullptr);
                  }
               }

               vkCmdDrawIndexed(aRenderDescriptor->mCommandBuffer, static_cast<uint32_t>(node->mMesh[i].mCount), 1, node->mMesh[i].mStartIndex, 0, 0);
            }
         }
      }
   }
   _VulkanManager->DebugMarkerEnd(aRenderDescriptor->mCommandBuffer);

}

void Model::Destroy() {
   for (size_t i = 0; i < mImages.size(); i++) {
      mImages[i].Destroy();
   }
   mVertexBuffer.Destroy();
   mIndexBuffer.Destroy();
   mVertices.clear();
   mIndices.clear();
}
