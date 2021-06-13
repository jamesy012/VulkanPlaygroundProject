#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


class Transform {
public:
	Transform();
	~Transform();

	void SetMatrix(glm::mat4 aNewMatrix);
	void SetPosition(glm::vec3 aNewPosition);
	void SetRotation(glm::vec3 aNewRotation);
	void SetRotation(glm::quat aNewRotation);
	void SetScale(glm::vec3 aNewScale);
	inline void SetScale(float aNewScale) {
		SetScale(glm::vec3(aNewScale));
	}

	void SetLookAt(glm::vec3 aPos, glm::vec3 aAt, glm::vec3 aUp);

	void SetParent(Transform* aParent);

	const glm::mat4 GetGlobalMatrix();
	const glm::mat4 GetLocalMatrix();

	const glm::vec3 GetPostion() const;
	const glm::vec3 GetRotation() const;
	const glm::quat GetRotationQuat() const;
	const glm::vec3 GetScale() const;

	void SetDirty();
	inline const bool IsDirty() const {
		return mIsDirty;
	}
	void Reset();

protected:
	virtual void UpdateModelMatrix();

	glm::vec3 mPosition;
	glm::vec3 mRotation;
	glm::vec3 mScale;
private:
	void RemoveChild(Transform* aChild);
	void RemoveParent();
	void AddParent(Transform* aParent);

	bool mIsDirty = true;
	bool mIsDirtyGlobal = true;
	glm::mat4 mLocalMatrix = glm::identity<glm::mat4>();
	glm::mat4 mGlobalMatrix = glm::identity<glm::mat4>();

	Transform* mParent = nullptr;
	std::vector<Transform*> mChildren;
};
