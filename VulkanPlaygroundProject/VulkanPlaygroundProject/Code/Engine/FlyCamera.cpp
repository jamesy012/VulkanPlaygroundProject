#include "stdafx.h"
#include "FlyCamera.h"

#include "InputHandler.h"
#include "glm/glm.hpp"

#include "imgui.h"

FlyCamera::FlyCamera() {
	mMovementSpeed = 30.0f;
	mTranslateSpeed = 1.0f;
	mRotationSpeed = 100.0f;
}


FlyCamera::~FlyCamera() {
}


void FlyCamera::UpdateInput() {

	float yaw = mRotation.y;
	float pitch = mRotation.x;
	
	float deltaTime = ImGui::GetIO().DeltaTime;

	glm::vec3 movement = glm::vec3(0);

	if (_CInput->IsKeyDown(IKEY_W)) {
		movement.z -= mMovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_S)) {
		movement.z += mMovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_Q)) {
		movement.y -= mMovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_E)) {
		movement.y += mMovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_A)) {
		movement.x -= mMovementSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_D)) {
		movement.x += mMovementSpeed * deltaTime;
	}

	bool leftMouse = _CInput->IsMouseKeyDown(IMOUSEKEY_LEFTBUTTON);
	bool rightMouse = _CInput->IsMouseKeyDown(IMOUSEKEY_RIGHTBUTTON);

	if (_CInput->IsKeyDown(IKEY_LeftArrow)) {
		mRotation.y += mRotationSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_RightArrow)) {
		mRotation.y -= mRotationSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_DownArrow)) {
		mRotation.x -= mRotationSpeed * deltaTime;
	}
	if (_CInput->IsKeyDown(IKEY_UpArrow)) {
		mRotation.x += mRotationSpeed * deltaTime;
	}

	if (_CInput->IsKeyDown(IKEY_Z)) {
		SetFov(GetFov() + 15.0f * deltaTime);
	}
	if (_CInput->IsKeyDown(IKEY_C)) {
		SetFov(GetFov() - 15.0f * deltaTime);
	}

	if (!leftMouse && rightMouse) {
		glm::vec2 mouseDelta = { _CInput->GetMouseDelta().y, _CInput->GetMouseDelta().x };
		mRotation += glm::vec3((mouseDelta * -0.2f), 0);
	}

	if (leftMouse && !rightMouse) {
		glm::vec2 mouseDelta = { _CInput->GetMouseDelta().y, _CInput->GetMouseDelta().x };
		movement.z += mouseDelta.x * mTranslateSpeed * 0.05f;
		mRotation.y += (mouseDelta.y * -0.2f);
	}

	if (leftMouse && rightMouse) {
		glm::vec2 mouseDelta = { _CInput->GetMouseDelta().y, _CInput->GetMouseDelta().x };
		movement.y -= mouseDelta.x * mTranslateSpeed * 0.05f;
		movement.x += mouseDelta.y * mTranslateSpeed * 0.05f;
	}

	movement.z -= _CInput->GetMouseScroll() * mTranslateSpeed * 0.01f;

	mPosition = glm::translate(GetLocalMatrix(), movement)[3];

	SetDirty();

	//SetLookAt(mPosition, glm::vec3(0), glm::vec3(0, 1, 0));

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