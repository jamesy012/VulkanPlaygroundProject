#include "stdafx.h"
#include "Transform.h"

#include <glm\ext.hpp>

Transform::Transform() {
	Reset();
}

Transform::~Transform() {
}

void Transform::SetPosition(glm::vec3 aNewPosition) {
	mPosition = aNewPosition;
	SetDirty();
}

void Transform::SetRotation(glm::vec3 aNewRotation) {
	mRotation = aNewRotation;
	SetDirty();
}

void Transform::SetRotation(glm::quat aNewRotation) {
#if defined(USE_QUATERNIONS)
	m_Rotation = a_Quat;
#else
	mRotation = glm::degrees(glm::eulerAngles(aNewRotation));
#endif // USE_QUATERNIONS

	SetDirty();
}

void Transform::SetScale(glm::vec3 aNewScale) {
	mScale = aNewScale;
	SetDirty();
}

void Transform::SetLookAt(glm::vec3 aPos, glm::vec3 aAt, glm::vec3 aUp) {
	glm::mat4 lookAt = glm::lookAt(aPos, aAt, aUp);

	glm::quat rotation = glm::quat(glm::inverse(lookAt));

#if defined(USE_QUATERNIONS)
	setRotation(rotation);
#else
	glm::vec3 quatRot = glm::degrees(glm::eulerAngles(rotation));
	if (abs(quatRot.z) > 90) {//if 180??!
		glm::vec3 euler;
		euler.x = quatRot.x - 180;
		euler.y = 180 - quatRot.y;
		euler.z = 0;
		SetRotation(euler);
	} else {
		SetRotation(rotation);
	}
#endif // USE_QUATERNIONS

	SetPosition(aPos);
	//SetScale(glm::vec3(1));
}

const glm::mat4 Transform::GetModelMatrix() {
	if (mIsDirty) {
		UpdateModelMatrix();
	}
	return mModelMatrix;
}

const glm::vec3 Transform::GetPostion() const {
	return mPosition;
}

const glm::vec3 Transform::GetRotation() const {
#if defined(USE_QUATERNIONS)
	return glm::degrees(glm::eulerAngles(mRotation));
#else
	return mRotation;
#endif
}

const glm::quat Transform::GetRotationQuat() const {
#if defined(USE_QUATERNIONS)
	return mRotation;
#else
	return glm::quat(mRotation);
#endif
}

const glm::vec3 Transform::GetScale() const {
	return mScale;
}

void Transform::Reset() {
	mPosition = glm::vec3(0);
#if defined(USE_QUATERNIONS)
	mRotation = glm::quat(0);
#else
	mRotation = glm::vec3(0);
#endif
	mScale = glm::vec3(1);
}

void Transform::UpdateModelMatrix() {
	mModelMatrix = glm::translate(glm::mat4(1), mPosition);
#if defined(USE_QUATERNIONS)
	mModelMatrix *= mRotation;
#else
	glm::mat4 rotmat = glm::mat4(1);
	rotmat = glm::rotate(rotmat, glm::radians(mRotation.y), glm::vec3(0, 1, 0));
	rotmat = glm::rotate(rotmat, glm::radians(mRotation.x), glm::vec3(1, 0, 0));
	rotmat = glm::rotate(rotmat, glm::radians(mRotation.z), glm::vec3(0, 0, 1));
	mModelMatrix *= rotmat;
#endif

	mModelMatrix = glm::scale(mModelMatrix, mScale);

	mIsDirty = false;
}
