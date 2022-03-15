#include "stdafx.h"
#include "Transform.h"

#include <glm/ext.hpp>
#include <glm/gtx/matrix_decompose.hpp>

Transform::Transform() {
	Reset();
}

Transform::~Transform() {
}

void Transform::SetMatrix(glm::mat4 aNewMatrix) {
	glm::quat rotation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(aNewMatrix, mScale, rotation, mPosition, skew, perspective);
	SetRotation(glm::conjugate(rotation));
	SetDirty();
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

void Transform::SetParent(Transform* aParent) {
	if (mParent == aParent) {
		return;
	}
	RemoveParent();
	AddParent(aParent);
}

const glm::mat4 Transform::GetGlobalMatrix() {
	if (mIsDirtyGlobal) {
		mGlobalMatrix = GetLocalMatrix();
		if (mParent) {
			mGlobalMatrix *= mParent->GetGlobalMatrix();
		}
		mIsDirtyGlobal = false;
	}
	return mGlobalMatrix;
}

const glm::mat4 Transform::GetLocalMatrix() {
	if (IsDirty()) {
		UpdateModelMatrix();
	}
	return mLocalMatrix;
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
	return glm::quat(glm::radians(mRotation));
#endif
}

const glm::vec3 Transform::GetScale() const {
	return mScale;
}

void Transform::SetDirty() {
	//if (!IsDirty()) {
		for (size_t i = 0; i < mChildren.size(); i++) {
			mChildren[i]->SetDirty();
		}
		mIsDirty = true;
		mIsDirtyGlobal = true;
	//}
}

void Transform::Reset() {
	mPosition = glm::vec3(0);
#if defined(USE_QUATERNIONS)
	mRotation = glm::quat(0);
#else
	mRotation = glm::vec3(0);
#endif
	mScale = glm::vec3(1);
	SetParent(nullptr);
}

void Transform::UpdateModelMatrix() {
	mLocalMatrix = glm::translate(glm::mat4(1), mPosition);
#if defined(USE_QUATERNIONS)
	mLocalMatrix *= glm::toMat4(mRotation);
#else
	glm::mat4 rotmat = glm::mat4(1);
	rotmat = glm::rotate(rotmat, glm::radians(mRotation.y), glm::vec3(0, 1, 0));
	rotmat = glm::rotate(rotmat, glm::radians(mRotation.x), glm::vec3(1, 0, 0));
	rotmat = glm::rotate(rotmat, glm::radians(mRotation.z), glm::vec3(0, 0, 1));
	mLocalMatrix *= rotmat;
#endif

	mLocalMatrix = glm::scale(mLocalMatrix, mScale);

	mIsDirty = false;
}

void Transform::RemoveChild(Transform* aChild) {
	for (size_t i = 0; i < mChildren.size(); i++) {
		if (mChildren[i] == aChild) {
			mChildren.erase(mChildren.begin() + i);
			return;
		}
	}
}

void Transform::RemoveParent() {
	if (mParent == nullptr) {
		return;
	}
	mParent->RemoveChild(this);
	mParent = nullptr;
	SetDirty();
}

void Transform::AddParent(Transform* aParent) {
	if (aParent == nullptr) {
		return;
	}
	mParent = aParent;
	mParent->mChildren.push_back(this);
	SetDirty();
}
