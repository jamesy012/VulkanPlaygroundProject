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

bool Model::LoadModel(std::string aPath, VkDescriptorSetLayout aMaterialDescriptorSet) {
   LOG_SCOPED_NAME("Model Loading");
   LOG("%s\n", aPath.c_str());
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

         if (!mMaterials[i].mDiffuse.empty()) {
            UpdateImageDescriptorSet(mMaterials[i].mDiffuse[0], mMaterials[i].mDescriptorSet, _VulkanManager->GetDefaultSampler(), 0);
         }
      }

   }

   LOG("Done\n");
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
               assert(false);
         }
      }

   }
}

void Model::Render(DescriptorUBO* aRenderDescriptor, RenderMode aRenderMode) {
   assert(aRenderMode != RenderMode::ALL);
   assert((aRenderMode & (aRenderMode-1)) == 0);
   if (!(mRenderModes & aRenderMode)) {
      return;
   }
   mVertexBuffer.Bind(aRenderDescriptor->mCommandBuffer);
   mIndexBuffer.Bind(aRenderDescriptor->mCommandBuffer);

   if (mDirty) {
      mBase->mRenderMatrix = glm::translate(glm::mat4(1), mPosition);
      mBase->mRenderMatrix *= glm::mat4(glm::quat(glm::radians(mRotation)));
      mBase->mRenderMatrix = glm::scale(mBase->mRenderMatrix, mScale);
      mDirty = false;
   }

   Render(aRenderDescriptor, aRenderMode, mBase, glm::identity<glm::mat4>());

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

void Model::Render(DescriptorUBO* aRenderDescriptor, RenderMode aRenderMode, Node* aNode, glm::mat4 aMatrix) {
   ObjectUBO ubo;
   ubo.m_Model = aMatrix * aNode->GetMatrix();

   if (!aNode->mMesh.empty()) {
      aRenderDescriptor->UpdateObjectAndBind(&ubo);

      for (int i = 0; i < aNode->mMesh.size(); i++) {
         Material& material = mMaterials[aNode->mMesh[i].mMaterialID];
         if (!material.mDiffuse.empty()) {
            vkCmdBindDescriptorSets(aRenderDescriptor->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, aRenderDescriptor->mPipelineLayout, 2, 1, &material.mDescriptorSet, 0, nullptr);
         }

         vkCmdDrawIndexed(aRenderDescriptor->mCommandBuffer, static_cast<uint32_t>(aNode->mMesh[i].mCount), 1, aNode->mMesh[i].mStartIndex, 0, 0);
      }
   }

   for (int i = 0; i < aNode->mChildren.size(); i++) {
      Render(aRenderDescriptor, aRenderMode, aNode->mChildren[i], ubo.m_Model);
   }
}
