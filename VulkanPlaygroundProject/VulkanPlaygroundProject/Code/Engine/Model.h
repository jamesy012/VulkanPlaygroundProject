#pragma once

#include "Vertex.h"
#include "Buffer.h"

#include "Image.h"
#include "Transform.h"

struct aiScene;
struct aiNode;

class Model {
public:
	//~~~~~~~~~~ NODE/MESH
	struct Mesh {
		Mesh() {};
		int mMaterialID = -1;
		int mCount = 0;
		int mStartIndex = 0;
		int mStartVertex = 0;
		glm::vec3 mMin = glm::zero<glm::vec3>();
		glm::vec3 mMax = glm::zero<glm::vec3>();
	};

	struct Node {
		Node() {
		}

		glm::mat4 GetMatrixWithParents() {
			return mTransform.GetGlobalMatrix();
		}

		glm::mat4 GetMatrix() {
			return mTransform.GetLocalMatrix();
		}

		std::string mName;

		Transform mTransform;
		Node* mParent = nullptr;
		std::vector<Node*> mChildren;
		std::vector<Mesh> mMesh;
	};

	enum class ImageType {
		DIFFUSE,
		NORMAL
	};

	struct Material {
		std::vector<Image*> mDiffuse;
		std::vector<Image*> mNormal;

		VkDescriptorSet mDescriptorSet;
	};

	struct ImageLoader {
		std::string mPath;
		std::vector<Material*> mMaterialRef;
		ImageType mType;
	};

	std::vector<ImageLoader> mImageLoader;
	std::vector<Image> mImages;//break out to image manager class?
	std::vector<Material> mMaterials;

public:
	bool LoadModel(std::string aPath, VkDescriptorSetLayout aMaterialDescriptorSet, std::vector<VkWriteDescriptorSet> aWriteSets);
	void Render(DescriptorUBO* aRenderDescriptor, RenderMode aRenderMode);

	void SetPosition(glm::vec3 aPos) {
		mBase.mTransform.SetPosition(aPos);
	}
	void SetRotation(glm::vec3 aRot) {
		mBase.mTransform.SetRotation(aRot);
	}
	void SetScale(float aScale) {
		mBase.mTransform.SetScale(aScale);
	}
	void SetScale(glm::vec3 aScale) {
		mBase.mTransform.SetScale(aScale);
	}
	glm::vec3 GetPosition() {
		return mBase.mTransform.GetPostion();
	}
	glm::vec3 GetRotation() {
		return mBase.mTransform.GetRotation();
	}
	glm::vec3 GetScale() {
		return mBase.mTransform.GetScale();
	}
	glm::mat4 GetMatrix() {
		return mBase.GetMatrixWithParents();
	}

	void SetRenderMode(unsigned int aMode) {
		mRenderModes = aMode;
	}
	void SetRenderMode(RenderMode aMode, bool aState) {
		if(aState) {
			AddRenderMode(aMode);
		} else {
			RemoveRenderMode(aMode);
		}
	}
	bool IsRenderMode(unsigned int aMode) {
		return mRenderModes & aMode;
	}

	void RemoveRenderMode(unsigned int aMode) {
		mRenderModes = mRenderModes & ~aMode;
	}
	void AddRenderMode(unsigned int aMode) {
		mRenderModes |= aMode;
	}

	Node* GetNode(size_t aIndex) {
		return mNodes[aIndex];
	}
	size_t GetNumNodes() {
		return mNodes.size();
	}

	void Destroy();

	~Model() {
		if(mVertices.size() != 0) {
			Destroy();
		}
	}
protected:
	std::vector<Vertex> mVertices;
	std::vector<uint32_t> mIndices;
private:
	void ProcessMeshs(const aiScene* aScene);
	void ProcessMesh(const aiScene* aScene, const aiNode* aNode, Node* aParent);
	void ProcessMaterials(const aiScene* aScene);
	void LoadImages();

	std::vector<Mesh*> mMeshs;
	Node mBase;
	std::vector<Node*> mNodes;

	unsigned int mRenderModes = RenderMode::ALL;

	std::string mName;
	std::string mPath;
	std::string mTexturePath;

	BufferVertex mVertexBuffer;
	BufferIndex mIndexBuffer;

	//friends
	friend class RenderManager;
};
