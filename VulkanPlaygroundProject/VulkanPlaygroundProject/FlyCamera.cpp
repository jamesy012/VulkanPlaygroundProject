#include "stdafx.h"
#include "FlyCamera.h"

#include "InputHandler.h"
#include "glm/glm.hpp"

#include "imgui.h"

FlyCamera::FlyCamera() {
	m_MovementSpeed = 30.0f;
	m_TranslateSpeed = 1.0f;
	m_RotationSpeed = 100.0f;
}


FlyCamera::~FlyCamera() {
}


void FlyCamera::UpdateInput() {

	float yaw = m_Rotation.y;
	float pitch = m_Rotation.x;
	
	float deltaTime = ImGui::GetIO().DeltaTime;

	glm::vec3 movement = glm::vec3(0);

	if (_CInput->IsKeyDown(IKEY_W)) {
		movement.z -= m_MovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_S)) {
		movement.z += m_MovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_Q)) {
		movement.y -= m_MovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_E)) {
		movement.y += m_MovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_A)) {
		movement.x -= m_MovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_D)) {
		movement.x += m_MovementSpeed * deltaTime;
	}

	bool leftMouse = _CInput->IsMouseKeyDown(IMOUSEKEY_LEFTBUTTON);
	bool rightMouse = _CInput->IsMouseKeyDown(IMOUSEKEY_RIGHTBUTTON);

	if (_CInput->IsKeyDown(IKEY_LeftArrow)) {
		m_Rotation.y += m_RotationSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_RightArrow)) {
		m_Rotation.y -= m_RotationSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_DownArrow)) {
		m_Rotation.x -= m_RotationSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_UpArrow)) {
		m_Rotation.x += m_RotationSpeed * deltaTime;
	}

	if (_CInput->IsKeyDown(IKEY_Z)) {
		SetFov(GetFov() + 15.0f * deltaTime);
	}
	if (_CInput->IsKeyDown(IKEY_C)) {
		SetFov(GetFov() - 15.0f * deltaTime);
	}

	if (!leftMouse && rightMouse) {
		glm::vec2 mouseDelta = { _CInput->GetMouseDelta().y, _CInput->GetMouseDelta().x };
		m_Rotation += glm::vec3((mouseDelta * -0.2f), 0);
	}

	if (leftMouse && !rightMouse) {
		glm::vec2 mouseDelta = { _CInput->GetMouseDelta().y, _CInput->GetMouseDelta().x };
		movement.z += mouseDelta.x * m_TranslateSpeed * 0.05f;
		m_Rotation.y += (mouseDelta.y * -0.2f);
	}

	if (leftMouse && rightMouse) {
		glm::vec2 mouseDelta = { _CInput->GetMouseDelta().y, _CInput->GetMouseDelta().x };
		movement.y -= mouseDelta.x * m_TranslateSpeed * 0.05f;
		movement.x += mouseDelta.y * m_TranslateSpeed * 0.05f;
	}

	movement.z -= _CInput->GetMouseScroll() * m_TranslateSpeed * 0.01f;

	m_Position = glm::translate(GetModelMatrix(), movement)[3];

	SetDirty();

	//SetLookAt(m_Position, glm::vec3(0), glm::vec3(0, 1, 0));

	//if (CameraLookAt) {
//	if (RotateCamera) {
//		mainCamera.SetLookAt(CameraPos + glm::vec3(x, y, 0), glm::vec3(0), glm::vec3(0, 1, 0));
//	} else {
//		mainCamera.SetLookAt(CameraPos, glm::vec3(0), glm::vec3(0,1,0));
//	}
//}else {
//	mainCamera.SetPosition(CameraPos);
//	mainCamera.SetRotation(glm::vec3(0));
//}

}