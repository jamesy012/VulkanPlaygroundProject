#pragma once

#include "Transform.h"

class Camera : public Transform {
public:
	Camera();

	Matrix GetView();
	Matrix GetProjection();
	Matrix GetPV();

	void SetAspectRatio(float a_NewAspect);
	void SetFov(float a_NewFov);
	void SetNearClip(float a_NearClip);
	void SetFarClip(float a_FarClip);

	float GetFov() const;

private:
	void UpdateModelMatrix() override;

	Matrix m_ProjectionMatrix;
	Matrix m_ViewMatrix;
	Matrix m_PV;

	float m_Aspect = 1.7f;
	float m_Fov = glm::radians(90.0f);
	float m_Near = 0.1f;
	float m_Far = 100.0f;
};
