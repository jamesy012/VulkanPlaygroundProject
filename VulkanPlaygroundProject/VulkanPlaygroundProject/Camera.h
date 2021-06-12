#pragma once

#include "Transform.h"

class Camera : public Transform {
public:
	Camera();

	glm::mat4 GetView();
	glm::mat4 GetProjection();
	glm::mat4 GetPV();

	void SetAspectRatio(float aNewAspect);
	void SetFov(float aNewFov);
	void SetNearClip(float aNearClip);
	void SetFarClip(float aFarClip);

	float GetFov() const;

private:
	void UpdateModelMatrix() override;

	glm::mat4 mProjectionMatrix;
	glm::mat4 mViewMatrix;
	glm::mat4 mPV;

	float mAspect = 1.7f;
	float mFov = glm::radians(70.0f);
	float mNearClip = 0.1f;
	float mFarClip = 100.0f;
};
