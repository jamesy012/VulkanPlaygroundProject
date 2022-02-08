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

	std::string GetKeysDown() const;
	std::vector<char> GetKeysDownArray() const;

	bool IsKeyDown(IKeys aKey) const;
	bool WasKeyPressed(IKeys aKey) const;
	bool WasKeyReleased(IKeys aKey) const;
	glm::vec2 GetMousePos() const;
	glm::vec2 GetMouseDelta() const;
	bool IsMouseKeyDown(IMouseKeys aKey) const;
	bool WasMouseKeyPressed(IMouseKeys aKey) const;
	bool WasMouseKeyReleased(IMouseKeys aKey) const;
	float GetMouseScroll() const;

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

