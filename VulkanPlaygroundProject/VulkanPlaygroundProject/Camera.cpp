#include "stdafx.h"
#include "Camera.h"

void Camera::UpdateModelMatrix() {
	Transform::UpdateModelMatrix();
	m_ViewMatrix = glm::inverse(m_ModelMatrix);
	m_ProjectionMatrix = glm::perspective(m_Fov, m_Aspect, m_Near, m_Far);
	m_ProjectionMatrix[1][1] *= -1;//vulkan flip
	m_PV = m_ProjectionMatrix * m_ViewMatrix;
}

Camera::Camera() {
}

Matrix Camera::GetView() {
	if (m_IsDirty) {
		UpdateModelMatrix();
	}
	return m_ViewMatrix;
}

Matrix Camera::GetProjection() {
	if (m_IsDirty) {
		UpdateModelMatrix();
	}
	return m_ProjectionMatrix;
}

Matrix Camera::GetPV() {
	if (m_IsDirty) {
		UpdateModelMatrix();
	}
	return m_PV;
}

void Camera::SetAspectRatio(float a_NewAspect) {
	m_Aspect = a_NewAspect;
	SetDirty();
}

void Camera::SetFov(float a_NewFov) {
	m_Fov = glm::clamp(glm::radians(a_NewFov), 0.1f, 179.9f);
	SetDirty();
}

void Camera::SetNearClip(float a_NearClip) {
	m_Near = a_NearClip;
	SetDirty();
}

void Camera::SetFarClip(float a_FarClip) {
	m_Far = a_FarClip;
	SetDirty();
}

float Camera::GetFov() const {
	return glm::degrees(m_Fov);
}