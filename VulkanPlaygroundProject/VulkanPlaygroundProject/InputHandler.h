#pragma once

#include "InputKeys.h"

#include <GLFW/glfw3.h>
#include <string>
#include <glm/glm.hpp>

extern class InputHandler* _CInput;
 
class InputHandler {
public:
	InputHandler();
	~InputHandler();

	bool Startup(GLFWwindow* aInputWindow);
	void Shutdown();

	void Update();

	std::string GetKeysDown();

	bool IsKeyDown(IKeys aKey);
	bool WasKeyPressed(IKeys aKey);
	glm::vec2 GetMousePos();
	glm::vec2 GetMouseDelta();
	bool IsMouseKeyDown(IMouseKeys aKey);
	bool WasMouseKeyPressed(IMouseKeys aKey);
	float GetMouseScroll();

	void AllowInput(bool aShouldProcessInput) {
		mGetNewValues = aShouldProcessInput;
	}
private:

	bool ReadKeyboard();
	bool ReadMouse();

	bool mGetNewValues = true;
	//delayed by one update to stop mouse jumping
	bool mDelayedGetNewValues = true;

};

