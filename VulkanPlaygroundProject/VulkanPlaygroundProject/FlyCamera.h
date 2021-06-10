#pragma once
#include "Camera.h"
class FlyCamera :
	public Camera {
public:
	FlyCamera();
	~FlyCamera();

	void UpdateInput();

	float m_MovementSpeed;
	float m_TranslateSpeed;
	float m_RotationSpeed;
};

