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

	bool Startup(GLFWwindow* a_InputWindow);
	void Shutdown();

	void Update();

	std::string GetKeysDown();

	bool IsKeyDown(IKeys a_Key);
	bool WasKeyPressed(IKeys a_Key);
	glm::vec2 GetMousePos();
	glm::vec2 GetMouseDelta();
	bool IsMouseKeyDown(IMouseKeys a_Key);
	bool WasMouseKeyPressed(IMouseKeys a_Key);
	float GetMouseScroll();

	void AllowInput(bool a_ShouldProcessInput) {
		m_GetNewValues = a_ShouldProcessInput;
	}
private:

	bool ReadKeyboard();
	bool ReadMouse();

	bool m_GetNewValues = true;

};

