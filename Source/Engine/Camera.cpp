#include "stdafx.h"
#include "Camera.h"

void Camera::UpdateModelMatrix() {
	Transform::UpdateModelMatrix();
	mViewMatrix = glm::inverse(GetGlobalMatrix());
	mProjectionMatrix = glm::perspective(mFov, mAspect, mNearClip, mFarClip);
	mProjectionMatrix[1][1] *= -1;//vulkan flip
	mPV = mProjectionMatrix * mViewMatrix;
}

Camera::Camera() {
	mPV = mProjectionMatrix = mViewMatrix = glm::identity<glm::mat4>();
}

glm::mat4 Camera::GetView() {
	if (IsDirty()) {
		UpdateModelMatrix();
	}
	return mViewMatrix;
}

glm::mat4 Camera::GetProjection() {
	if (IsDirty()) {
		UpdateModelMatrix();
	}
	return mProjectionMatrix;
}

glm::mat4 Camera::GetPV() {
	if (IsDirty()) {
		UpdateModelMatrix();
	}
	return mPV;
}

void Camera::SetAspectRatio(float aNewAspect) {
	mAspect = aNewAspect;
	SetDirty();
}

void Camera::SetFov(float aNewFov) {
	mFov = glm::clamp(glm::radians(aNewFov), 0.1f, 179.9f);
	SetDirty();
}

void Camera::SetNearClip(float aNearClip) {
	mNearClip = aNearClip;
	SetDirty();
}

void Camera::SetFarClip(float aFarClip) {
	mFarClip = aFarClip;
	SetDirty();
}