#include "stdafx.h"
#include "InputHandler.h"

#if WINDOWS
#define USE_DINPUT 1
#endif

//fallback on glfw?
#if !defined(USE_DINPUT)
#define USE_DINPUT 0
#define USE_GLFWINPUT 1
#else
#define USE_GLFWINPUT 0
#endif

#if USE_DINPUT
#define (unsigned char)GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

LPDIRECTINPUT8 mDirectInput;
LPDIRECTINPUTDEVICE8 mKeyboard;
LPDIRECTINPUTDEVICE8 mMouse;
DIMOUSESTATE mMouseState;
DIMOUSESTATE mMouseStateOld;

HWND hwnd;

#elif USE_GLFWINPUT
#include <GLFW/glfw3.h>
GLFWwindow* mWindow;
struct MouseStates{
	bool buttons[(uint)IMouseKeys::IMOUSEKEY_MAX_NUM_BUTTONS];
};
MouseStates mMouseState;
MouseStates mMouseStateOld;
#endif


unsigned char mKeyboardState[256];
unsigned char mKeyboardStateOld[256];
int mMouseX, mMouseY;
int mMouseDeltaX, mMouseDeltaY;

InputHandler* _CInput;

#if USE_GLFWINPUT
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	mMouseState.buttons[button] = action == (unsigned char)GLFW_PRESS;
}
void MousePosCallback(GLFWwindow* window, double xpos, double ypos) {
	mMouseDeltaX = mMouseX - xpos;
	mMouseDeltaY = mMouseY - ypos;
	mMouseX = xpos;
	mMouseY = ypos;
}
void KeyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	mKeyboardState[key] = action == (unsigned char)GLFW_PRESS;
}
#endif

unsigned char mKeyboardRemapper[256] = { 0 };

InputHandler::InputHandler() {
#if USE_DINPUT
	mKeyboardRemapper[IKEY_Escape] = DIK_ESCAPE;
	mKeyboardRemapper[IKEY_1] = DIK_1;
	mKeyboardRemapper[IKEY_2] = DIK_2;
	mKeyboardRemapper[IKEY_3] = DIK_3;
	mKeyboardRemapper[IKEY_4] = DIK_4;
	mKeyboardRemapper[IKEY_5] = DIK_5;
	mKeyboardRemapper[IKEY_6] = DIK_6;
	mKeyboardRemapper[IKEY_7] = DIK_7;
	mKeyboardRemapper[IKEY_8] = DIK_8;
	mKeyboardRemapper[IKEY_9] = DIK_9;
	mKeyboardRemapper[IKEY_0] = DIK_0;
	mKeyboardRemapper[IKEY_Minus] = DIK_MINUS;
	mKeyboardRemapper[IKEY_Equals] = DIK_EQUALS;
	mKeyboardRemapper[IKEY_Backspace] = DIK_BACK;
	mKeyboardRemapper[IKEY_Tab] = DIK_TAB;
	mKeyboardRemapper[IKEY_Q] = DIK_Q;
	mKeyboardRemapper[IKEY_W] = DIK_W;
	mKeyboardRemapper[IKEY_E] = DIK_E;
	mKeyboardRemapper[IKEY_R] = DIK_R;
	mKeyboardRemapper[IKEY_T] = DIK_T;
	mKeyboardRemapper[IKEY_Y] = DIK_Y;
	mKeyboardRemapper[IKEY_U] = DIK_U;
	mKeyboardRemapper[IKEY_I] = DIK_I;
	mKeyboardRemapper[IKEY_O] = DIK_O;
	mKeyboardRemapper[IKEY_P] = DIK_P;
	mKeyboardRemapper[IKEY_OpenSquareBracket] = DIK_LBRACKET;
	mKeyboardRemapper[IKEY_CloseSquareBracket] = DIK_RBRACKET;
	mKeyboardRemapper[IKEY_Enter] = DIK_RETURN;
	mKeyboardRemapper[IKEY_LeftCtrl] = DIK_LCONTROL;
	mKeyboardRemapper[IKEY_A] = DIK_A;
	mKeyboardRemapper[IKEY_S] = DIK_S;
	mKeyboardRemapper[IKEY_D] = DIK_D;
	mKeyboardRemapper[IKEY_F] = DIK_F;
	mKeyboardRemapper[IKEY_G] = DIK_G;
	mKeyboardRemapper[IKEY_H] = DIK_H;
	mKeyboardRemapper[IKEY_J] = DIK_J;
	mKeyboardRemapper[IKEY_K] = DIK_K;
	mKeyboardRemapper[IKEY_L] = DIK_L;
	mKeyboardRemapper[IKEY_SemiColon] = DIK_SEMICOLON;
	mKeyboardRemapper[IKEY_Apostrophe] = DIK_APOSTROPHE;
	mKeyboardRemapper[IKEY_Tilde] = DIK_GRAVE;
	mKeyboardRemapper[IKEY_LeftShift] = DIK_LSHIFT;
	mKeyboardRemapper[IKEY_BackSlash] = DIK_BACKSLASH;
	mKeyboardRemapper[IKEY_Z] = DIK_Z;
	mKeyboardRemapper[IKEY_X] = DIK_X;
	mKeyboardRemapper[IKEY_C] = DIK_C;
	mKeyboardRemapper[IKEY_V] = DIK_V;
	mKeyboardRemapper[IKEY_B] = DIK_B;
	mKeyboardRemapper[IKEY_N] = DIK_N;
	mKeyboardRemapper[IKEY_M] = DIK_M;
	mKeyboardRemapper[IKEY_Comma] = DIK_COMMA;
	mKeyboardRemapper[IKEY_Period] = DIK_PERIOD;
	mKeyboardRemapper[IKEY_ForwardSlash] = DIK_SLASH;
	mKeyboardRemapper[IKEY_RightShift] = DIK_RSHIFT;
	mKeyboardRemapper[IKEY_PAD_Asterisk] = DIK_MULTIPLY;
	mKeyboardRemapper[IKEY_Space] = DIK_SPACE;
	mKeyboardRemapper[IKEY_CapsLock] = DIK_CAPITAL;
	mKeyboardRemapper[IKEY_F1] = DIK_F1;
	mKeyboardRemapper[IKEY_F2] = DIK_F2;
	mKeyboardRemapper[IKEY_F3] = DIK_F3;
	mKeyboardRemapper[IKEY_F4] = DIK_F4;
	mKeyboardRemapper[IKEY_F5] = DIK_F5;
	mKeyboardRemapper[IKEY_F6] = DIK_F6;
	mKeyboardRemapper[IKEY_F7] = DIK_F7;
	mKeyboardRemapper[IKEY_F8] = DIK_F8;
	mKeyboardRemapper[IKEY_F9] = DIK_F9;
	mKeyboardRemapper[IKEY_F10] = DIK_F10;
	mKeyboardRemapper[IKEY_NumLock] = DIK_NUMLOCK;
	mKeyboardRemapper[IKEY_ScrollLock] = DIK_SCROLL;
	mKeyboardRemapper[IKEY_PAD_7] = DIK_NUMPAD7;
	mKeyboardRemapper[IKEY_PAD_8] = DIK_NUMPAD8;
	mKeyboardRemapper[IKEY_PAD_9] = DIK_NUMPAD9;
	mKeyboardRemapper[IKEY_PAD_Minus] = DIK_SUBTRACT;
	mKeyboardRemapper[IKEY_PAD_4] = DIK_NUMPAD4;
	mKeyboardRemapper[IKEY_PAD_5] = DIK_NUMPAD5;
	mKeyboardRemapper[IKEY_PAD_6] = DIK_NUMPAD6;
	mKeyboardRemapper[IKEY_PAD_Plus] = DIK_ADD;
	mKeyboardRemapper[IKEY_PAD_1] = DIK_NUMPAD1;
	mKeyboardRemapper[IKEY_PAD_2] = DIK_NUMPAD2;
	mKeyboardRemapper[IKEY_PAD_3] = DIK_NUMPAD3;
	mKeyboardRemapper[IKEY_PAD_0] = DIK_NUMPAD0;
	mKeyboardRemapper[IKEY_PAD_Period] = DIK_DECIMAL;
	mKeyboardRemapper[IKEY_F11] = DIK_F11;
	mKeyboardRemapper[IKEY_F12] = DIK_F12;
	mKeyboardRemapper[IKEY_PAD_Enter] = DIK_NUMPADENTER;
	mKeyboardRemapper[IKEY_RightCtrl] = DIK_RCONTROL;
	mKeyboardRemapper[IKEY_PAD_ForwardSlash] = DIK_DIVIDE;
	mKeyboardRemapper[IKEY_Home] = DIK_HOME;
	mKeyboardRemapper[IKEY_UpArrow] = DIK_UP;
	mKeyboardRemapper[IKEY_LeftArrow] = DIK_LEFT;
	mKeyboardRemapper[IKEY_RightArrow] = DIK_RIGHT;
	mKeyboardRemapper[IKEY_End] = DIK_END;
	mKeyboardRemapper[IKEY_DownArrow] = DIK_DOWN;
	mKeyboardRemapper[IKEY_Insert] = DIK_INSERT;
	mKeyboardRemapper[IKEY_Delete] = DIK_DELETE;
#elif USE_GLFWINPUT
	mKeyboardRemapper[IKEY_Escape] = (unsigned char)GLFW_KEY_ESCAPE;
	mKeyboardRemapper[IKEY_1] = (unsigned char)GLFW_KEY_1;
	mKeyboardRemapper[IKEY_2] = (unsigned char)GLFW_KEY_2;
	mKeyboardRemapper[IKEY_3] = (unsigned char)GLFW_KEY_3;
	mKeyboardRemapper[IKEY_4] = (unsigned char)GLFW_KEY_4;
	mKeyboardRemapper[IKEY_5] = (unsigned char)GLFW_KEY_5;
	mKeyboardRemapper[IKEY_6] = (unsigned char)GLFW_KEY_6;
	mKeyboardRemapper[IKEY_7] = (unsigned char)GLFW_KEY_7;
	mKeyboardRemapper[IKEY_8] = (unsigned char)GLFW_KEY_8;
	mKeyboardRemapper[IKEY_9] = (unsigned char)GLFW_KEY_9;
	mKeyboardRemapper[IKEY_0] = (unsigned char)GLFW_KEY_0;
	mKeyboardRemapper[IKEY_Minus] = (unsigned char)GLFW_KEY_MINUS;
	mKeyboardRemapper[IKEY_Equals] = (unsigned char)GLFW_KEY_EQUAL;
	mKeyboardRemapper[IKEY_Backspace] = (unsigned char)GLFW_KEY_BACKSPACE;
	mKeyboardRemapper[IKEY_Tab] = (unsigned char)GLFW_KEY_TAB;
	mKeyboardRemapper[IKEY_Q] = (unsigned char)GLFW_KEY_Q;
	mKeyboardRemapper[IKEY_W] = (unsigned char)GLFW_KEY_W;
	mKeyboardRemapper[IKEY_E] = (unsigned char)GLFW_KEY_E;
	mKeyboardRemapper[IKEY_R] = (unsigned char)GLFW_KEY_R;
	mKeyboardRemapper[IKEY_T] = (unsigned char)GLFW_KEY_T;
	mKeyboardRemapper[IKEY_Y] = (unsigned char)GLFW_KEY_Y;
	mKeyboardRemapper[IKEY_U] = (unsigned char)GLFW_KEY_U;
	mKeyboardRemapper[IKEY_I] = (unsigned char)GLFW_KEY_I;
	mKeyboardRemapper[IKEY_O] = (unsigned char)GLFW_KEY_O;
	mKeyboardRemapper[IKEY_P] = (unsigned char)GLFW_KEY_P;
	mKeyboardRemapper[IKEY_OpenSquareBracket] = (unsigned char)GLFW_KEY_LEFT_BRACKET;
	mKeyboardRemapper[IKEY_CloseSquareBracket] = (unsigned char)GLFW_KEY_RIGHT_BRACKET;
	mKeyboardRemapper[IKEY_Enter] = (unsigned char)GLFW_KEY_ENTER;
	mKeyboardRemapper[IKEY_LeftCtrl] = (unsigned char)GLFW_KEY_LEFT_CONTROL;
	mKeyboardRemapper[IKEY_A] = (unsigned char)GLFW_KEY_A;
	mKeyboardRemapper[IKEY_S] = (unsigned char)GLFW_KEY_S;
	mKeyboardRemapper[IKEY_D] = (unsigned char)GLFW_KEY_D;
	mKeyboardRemapper[IKEY_F] = (unsigned char)GLFW_KEY_F;
	mKeyboardRemapper[IKEY_G] = (unsigned char)GLFW_KEY_G;
	mKeyboardRemapper[IKEY_H] = (unsigned char)GLFW_KEY_H;
	mKeyboardRemapper[IKEY_J] = (unsigned char)GLFW_KEY_J;
	mKeyboardRemapper[IKEY_K] = (unsigned char)GLFW_KEY_K;
	mKeyboardRemapper[IKEY_L] = (unsigned char)GLFW_KEY_L;
	mKeyboardRemapper[IKEY_SemiColon] = (unsigned char)GLFW_KEY_SEMICOLON;
	mKeyboardRemapper[IKEY_Apostrophe] = (unsigned char)GLFW_KEY_APOSTROPHE;
	mKeyboardRemapper[IKEY_Tilde] = (unsigned char)GLFW_KEY_GRAVE_ACCENT;
	mKeyboardRemapper[IKEY_LeftShift] = (unsigned char)GLFW_KEY_LEFT_SHIFT;
	mKeyboardRemapper[IKEY_BackSlash] = (unsigned char)GLFW_KEY_BACKSLASH;
	mKeyboardRemapper[IKEY_Z] = (unsigned char)GLFW_KEY_Z;
	mKeyboardRemapper[IKEY_X] = (unsigned char)GLFW_KEY_X;
	mKeyboardRemapper[IKEY_C] = (unsigned char)GLFW_KEY_C;
	mKeyboardRemapper[IKEY_V] = (unsigned char)GLFW_KEY_V;
	mKeyboardRemapper[IKEY_B] = (unsigned char)GLFW_KEY_B;
	mKeyboardRemapper[IKEY_N] = (unsigned char)GLFW_KEY_N;
	mKeyboardRemapper[IKEY_M] = (unsigned char)GLFW_KEY_M;
	mKeyboardRemapper[IKEY_Comma] = (unsigned char)GLFW_KEY_COMMA;
	mKeyboardRemapper[IKEY_Period] = (unsigned char)GLFW_KEY_PERIOD;
	mKeyboardRemapper[IKEY_ForwardSlash] = (unsigned char)GLFW_KEY_SLASH;
	mKeyboardRemapper[IKEY_RightShift] = (unsigned char)GLFW_KEY_RIGHT_SHIFT;
	mKeyboardRemapper[IKEY_PAD_Asterisk] = (unsigned char)GLFW_KEY_KP_MULTIPLY;
	mKeyboardRemapper[IKEY_Space] = (unsigned char)GLFW_KEY_SPACE;
	mKeyboardRemapper[IKEY_CapsLock] = (unsigned char)GLFW_KEY_CAPS_LOCK;
	mKeyboardRemapper[IKEY_F1] = (unsigned char)GLFW_KEY_F1;
	mKeyboardRemapper[IKEY_F2] = (unsigned char)GLFW_KEY_F2;
	mKeyboardRemapper[IKEY_F3] = (unsigned char)GLFW_KEY_F3;
	mKeyboardRemapper[IKEY_F4] = (unsigned char)GLFW_KEY_F4;
	mKeyboardRemapper[IKEY_F5] = (unsigned char)GLFW_KEY_F5;
	mKeyboardRemapper[IKEY_F6] = (unsigned char)GLFW_KEY_F6;
	mKeyboardRemapper[IKEY_F7] = (unsigned char)GLFW_KEY_F7;
	mKeyboardRemapper[IKEY_F8] = (unsigned char)GLFW_KEY_F8;
	mKeyboardRemapper[IKEY_F9] = (unsigned char)GLFW_KEY_F9;
	mKeyboardRemapper[IKEY_F10] = (unsigned char)GLFW_KEY_F10;
	mKeyboardRemapper[IKEY_NumLock] = (unsigned char)GLFW_KEY_NUM_LOCK;
	mKeyboardRemapper[IKEY_ScrollLock] = (unsigned char)GLFW_KEY_SCROLL_LOCK;
	mKeyboardRemapper[IKEY_PAD_7] = (unsigned char)GLFW_KEY_KP_7;
	mKeyboardRemapper[IKEY_PAD_8] = (unsigned char)GLFW_KEY_KP_8;
	mKeyboardRemapper[IKEY_PAD_9] = (unsigned char)GLFW_KEY_KP_9;
	mKeyboardRemapper[IKEY_PAD_Minus] = (unsigned char)GLFW_KEY_KP_SUBTRACT;
	mKeyboardRemapper[IKEY_PAD_4] = (unsigned char)GLFW_KEY_KP_4;
	mKeyboardRemapper[IKEY_PAD_5] = (unsigned char)GLFW_KEY_KP_5;
	mKeyboardRemapper[IKEY_PAD_6] = (unsigned char)GLFW_KEY_KP_6;
	mKeyboardRemapper[IKEY_PAD_Plus] = (unsigned char)GLFW_KEY_KP_ADD;
	mKeyboardRemapper[IKEY_PAD_1] = (unsigned char)GLFW_KEY_KP_1;
	mKeyboardRemapper[IKEY_PAD_2] = (unsigned char)GLFW_KEY_KP_2;
	mKeyboardRemapper[IKEY_PAD_3] = (unsigned char)GLFW_KEY_KP_3;
	mKeyboardRemapper[IKEY_PAD_0] = (unsigned char)GLFW_KEY_KP_0;
	mKeyboardRemapper[IKEY_PAD_Period] = (unsigned char)GLFW_KEY_KP_DECIMAL;
	mKeyboardRemapper[IKEY_F11] = (unsigned char)GLFW_KEY_F11;
	mKeyboardRemapper[IKEY_F12] = (unsigned char)GLFW_KEY_F12;
	mKeyboardRemapper[IKEY_PAD_Enter] = (unsigned char)GLFW_KEY_KP_ENTER;
	mKeyboardRemapper[IKEY_RightCtrl] = (unsigned char)GLFW_KEY_RIGHT_CONTROL;
	mKeyboardRemapper[IKEY_PAD_ForwardSlash] = (unsigned char)GLFW_KEY_KP_DIVIDE;
	mKeyboardRemapper[IKEY_Home] = (unsigned char)GLFW_KEY_HOME;
	mKeyboardRemapper[IKEY_UpArrow] = (unsigned char)GLFW_KEY_UP;
	mKeyboardRemapper[IKEY_LeftArrow] = (unsigned char)GLFW_KEY_LEFT;
	mKeyboardRemapper[IKEY_RightArrow] = (unsigned char)GLFW_KEY_RIGHT;
	mKeyboardRemapper[IKEY_End] = (unsigned char)GLFW_KEY_END;
	mKeyboardRemapper[IKEY_DownArrow] = (unsigned char)GLFW_KEY_DOWN;
	mKeyboardRemapper[IKEY_Insert] = (unsigned char)GLFW_KEY_INSERT;
	mKeyboardRemapper[IKEY_Delete] = (unsigned char)GLFW_KEY_DELETE;
#endif
}

InputHandler::~InputHandler() {
}

bool InputHandler::Startup(GLFWwindow* a_InputWindow) {
#if USE_DINPUT
	HINSTANCE hInstance = GetModuleHandle(NULL);
	hwnd = glfwGetWin32Window(a_InputWindow);
	HRESULT hr;

	hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&mDirectInput, NULL);
	if (FAILED(hr)) {
		return false;
	}

	hr = mDirectInput->CreateDevice(GUID_SysKeyboard, &mKeyboard, NULL);
	if (FAILED(hr)) {
		return false;
	}

	hr = mKeyboard->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr)) {
		return false;
	}

	hr = mKeyboard->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);//DISCL_NONEXCLUSIVE | DISCL_BACKGROUND
	if (FAILED(hr)) {
		return false;
	}

	hr = mKeyboard->Acquire();
	if (FAILED(hr)) {
		return false;
	}

	hr = mDirectInput->CreateDevice(GUID_SysMouse, &mMouse, NULL);
	if (FAILED(hr)) {
		return false;
	}

	hr = mMouse->SetDataFormat(&c_dfDIMouse);
	if (FAILED(hr)) {
		return false;
	}

	hr = mMouse->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);//DISCL_NONEXCLUSIVE | DISCL_BACKGROUND
	if (FAILED(hr)) {
		return false;
	}

	hr = mMouse->Acquire();
	if (FAILED(hr)) {
		return false;
	}

#elif USE_GLFWINPUT
	mWindow = a_InputWindow;

	glfwSetMouseButtonCallback(mWindow, MouseButtonCallback);
	glfwSetCursorPosCallback(mWindow, MousePosCallback);
	glfwSetKeyCallback(mWindow, KeyPressCallback);
#endif

	_CInput = this;

	return true;
}

void InputHandler::Shutdown() {
#if USE_DINPUT
	if (mMouse) {
		mMouse->Unacquire();
		mMouse->Release();
		mMouse = nullptr;
	}

	if (mKeyboard) {
		mKeyboard->Unacquire();
		mKeyboard->Release();
		mKeyboard = nullptr;
	}

	if (mDirectInput) {
		mDirectInput->Release();
		mDirectInput = nullptr;
	}
#endif
}

void InputHandler::Update() {

	ReadKeyboard();
	ReadMouse();

	//// Update the location of the mouse cursor based on the change of the mouse location during the frame.
	//mMouseX += mMouseState.lX;
	//mMouseY += mMouseState.lY;
	//
	//// Ensure the mouse location doesn't exceed the screen width or height.
	//if (mMouseX < 0) { mMouseX = 0; }
	//if (mMouseY < 0) { mMouseY = 0; }
	//
	//if (mMouseX > m_MainWindow->m_WindowWidth) { mMouseX = m_MainWindow->m_WindowWidth; }
	//if (mMouseY > m_MainWindow->m_WindowHeight) { mMouseY = m_MainWindow->m_WindowHeight; }

	if (mDelayedGetNewValues != mGetNewValues) {
		mMouseDeltaX = mMouseDeltaY = 0;
		mMouseX = mMouseY = 0;
		mDelayedGetNewValues = mGetNewValues;
	}

}

bool InputHandler::IsKeyDown(IKeys aKey) const {
	return mKeyboardState[mKeyboardRemapper[aKey]];
}

bool InputHandler::WasKeyPressed(IKeys aKey) const {
	const int index = mKeyboardRemapper[aKey];
	return mKeyboardState[index] && !mKeyboardStateOld[index];
}

bool InputHandler::WasKeyReleased(IKeys aKey) const {
	const int index = mKeyboardRemapper[aKey];
	return !mKeyboardState[index] && mKeyboardStateOld[index];
}

glm::vec2 InputHandler::GetMousePos() const {
	return glm::vec2(mMouseX, mMouseY);
}

glm::vec2 InputHandler::GetMouseDelta() const {
	return glm::vec2(mMouseDeltaX, mMouseDeltaY);
}

bool InputHandler::IsMouseKeyDown(IMouseKeys aKey) const {
#if USE_DINPUT
	return mMouseState.rgbButtons[aKey] & 0x80;
#elif USE_GLFWINPUT
	return mMouseState.buttons[aKey];
#else
	return false;
#endif
}

bool InputHandler::WasMouseKeyPressed(IMouseKeys aKey) const {
#if USE_DINPUT
	return (mMouseState.rgbButtons[aKey] & 0x80) && !(mMouseStateOld.rgbButtons[aKey] & 0x80);
#elif USE_GLFWINPUT
	return mMouseState.buttons[aKey] && !mMouseStateOld.buttons[aKey];
#else
	return false;
#endif
}

bool InputHandler::WasMouseKeyReleased(IMouseKeys aKey) const {
#if USE_DINPUT
	return !(mMouseState.rgbButtons[aKey] & 0x80) && (mMouseStateOld.rgbButtons[aKey] & 0x80);
#elif USE_GLFWINPUT
	return !mMouseState.buttons[aKey] && mMouseStateOld.buttons[aKey];
#else
	return false;
#endif
}

float InputHandler::GetMouseScroll() const {
#if USE_DINPUT
	return (float)mMouseState.lZ;
#elif USE_GLFWINPUT
return false;//glfwGetScr;
#else
return false;
#endif
}

std::vector<char> InputHandler::GetKeysDownArray() const {
	std::vector<char> output;
	for (int i = 0; i < 10; i++) {
		if (IsKeyDown(IKeys(IKEY_0 + i))) {
			output.push_back('0' + i);
		}
	}
	for (int i = 0; i < 27; i++) {
		if (IsKeyDown(IKeys(IKEY_A + i))) {
			output.push_back('a' + i);
		}
	}

	return output;
}

std::string InputHandler::GetKeysDown() const {
	//only 0-9 and A-Z

	std::string output;

	for (int i = 0; i < 10; i++) {
		if (IsKeyDown(IKeys(IKEY_0 + i))) {
			output += std::to_string(i) + ",";
		}
	}

	for (int i = 0; i < 27; i++) {
		if (IsKeyDown(IKeys(IKEY_A + i))) {
			output += (char)('a' + i) + std::string(",");
		}
	}

	if (IsKeyDown(IKEY_RightArrow)) {
		output += std::string("Right,");
	}
	if (IsKeyDown(IKEY_LeftArrow)) {
		output += std::string("Left,");
	}
	if (IsKeyDown(IKEY_UpArrow)) {
		output += std::string("Up,");
	}
	if (IsKeyDown(IKEY_DownArrow)) {
		output += std::string("Down,");
	}

	if (IsKeyDown(IKEY_Escape)) {
		output += std::string("Escape,");
	}

	return output;
}


bool InputHandler::ReadKeyboard() {
#if USE_DINPUT
	HRESULT hr;

	memcpy(mKeyboardStateOld, mKeyboardState, sizeof(mKeyboardState));
	if (!mGetNewValues) {
		memset(mKeyboardState, 0, sizeof(mKeyboardState));
		return false;
	}
	// Read the keyboard device.
	hr = mKeyboard->GetDeviceState(sizeof(mKeyboardState), (LPVOID)&mKeyboardState);
	if (FAILED(hr)) {
		// If the keyboard lost focus or was not acquired then try to get control back.
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED)) {
			mKeyboard->Acquire();
		} else {
			return false;
		}
	}
	return true;
#elif USE_GLFWINPUT
	memcpy(&mKeyboardStateOld, &mKeyboardState, sizeof(mKeyboardState));
	//reset mKeyboardState? the callback doesnt trigger each frame it's active
	return true;
#endif
}

bool InputHandler::ReadMouse() {
#if USE_DINPUT
	HRESULT hr;


	POINT pos;
	if (::GetCursorPos(&pos) && ::ScreenToClient(hwnd, &pos)) {
		//mMouseDeltaX = m_MouseX - pos.x;
		//mMouseDeltaY = m_MouseY - pos.y;
		mMouseX = (int)pos.x;
		mMouseY = (int)pos.y;
	}

	if (mMouse == nullptr) {
		return false;
	}

	memcpy(&mMouseStateOld, &mMouseState, sizeof(mMouseState));
	if (!mGetNewValues) {
		memset(&mMouseState, 0, sizeof(mMouseState));
		return false;
	}
	// Read the mouse device.
	hr = mMouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mMouseState);
	if (FAILED(hr)) {
		// If the mouse lost focus or was not acquired then try to get control back.
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED)) {
			mMouse->Acquire();
		} else {
			return false;
		}
	} else {
		mMouseDeltaX = mMouseState.lX;
		mMouseDeltaY = mMouseState.lY;
	}
	return true;
#elif USE_GLFWINPUT
	memcpy(&mMouseStateOld, &mMouseState, sizeof(MouseStates));
	//reset mMouseState? the callback doesnt trigger each frame it's active
	return true;
#else
	return false;
#endif
}
