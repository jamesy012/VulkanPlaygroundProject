#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


class Transform {
public:
	Transform();
	~Transform();

	void SetPosition(glm::vec3 aNewPosition);
	void SetRotation(glm::vec3 aNewRotation);
	void SetRotation(glm::quat aNewRotation);
	void SetScale(glm::vec3 aNewScale);

	void SetLookAt(glm::vec3 aPos, glm::vec3 aAt, glm::vec3 aUp);

	const glm::mat4 GetModelMatrix();
	const glm::vec3 GetPostion() const;
	const glm::vec3 GetRotation() const;
	const glm::quat GetRotationQuat() const;
	const glm::vec3 GetScale() const;

	inline void SetDirty() {
		mIsDirty = true;
	}

	void Reset();

protected:
	glm::vec3 mPosition;
	glm::vec3 mRotation;
	glm::vec3 mScale;

	virtual void UpdateModelMatrix();

	bool mIsDirty = true;
	glm::mat4 mModelMatrix;
};
