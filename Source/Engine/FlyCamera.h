#pragma once
#include "Camera.h"
class FlyCamera :
	public Camera {
public:
	FlyCamera();
	~FlyCamera();

	void UpdateInput();

	float mMovementSpeed;
	float mTranslateSpeed;
	float mRotationSpeed;
};

