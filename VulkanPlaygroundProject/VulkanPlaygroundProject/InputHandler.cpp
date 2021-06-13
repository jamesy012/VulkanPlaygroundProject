#include "stdafx.h"
#include "InputHandler.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

InputHandler* _CInput;

LPDIRECTINPUT8 mDirectInput;
LPDIRECTINPUTDEVICE8 mKeyboard;
LPDIRECTINPUTDEVICE8 mMouse;

unsigned char mKeyboardState[256];
unsigned char mKeyboardStateOld[256];
DIMOUSESTATE mMouseState;
DIMOUSESTATE mMouseStateOld;
int mMouseX, mMouseY;
int mMouseDeltaX, mMouseDeltaY;

unsigned char mKeyboardRemapper[256] = { 0 };
//	IKEY_Escape				/*DIK_ESCAPE		*/
//	, IKEY_1					/*DIK_1				*/
//	, IKEY_2					/*DIK_2				*/
//	, IKEY_3					/*DIK_3				*/
//	, IKEY_4					/*DIK_4				*/
//	, IKEY_5					/*DIK_5				*/
//	, IKEY_6					/*DIK_6				*/
//	, IKEY_7					/*DIK_7				*/
//	, IKEY_8					/*DIK_8				*/
//	, IKEY_9					/*DIK_9				*/
//	, IKEY_0					/*DIK_0				*/
//	, IKEY_Minus				/*DIK_MINUS			*/
//	, IKEY_Equals				/*DIK_EQUALS		*/
//	, IKEY_Backspace			/*DIK_BACK			*/
//	, IKEY_Tab					/*DIK_TAB			*/
//	, IKEY_Q					/*DIK_Q				*/
//	, IKEY_W					/*DIK_W				*/
//	, IKEY_E					/*DIK_E				*/
//	, IKEY_R					/*DIK_R				*/
//	, IKEY_T					/*DIK_T				*/
//	, IKEY_Y					/*DIK_Y				*/
//	, IKEY_U					/*DIK_U				*/
//	, IKEY_I					/*DIK_I				*/
//	, IKEY_O					/*DIK_O				*/
//	, IKEY_P					/*DIK_P				*/
//	, IKEY_OpenSquareBracket	/*DIK_LBRACKET		*/
//	, IKEY_CloseSquareBracket	/*DIK_RBRACKET		*/
//	, IKEY_Enter				/*DIK_RETURN		*/
//	, IKEY_LeftCtrl				/*DIK_LCONTROL		*/
//	, IKEY_A					/*DIK_A				*/
//	, IKEY_S					/*DIK_S				*/
//	, IKEY_D					/*DIK_D				*/
//	, IKEY_F					/*DIK_F				*/
//	, IKEY_G					/*DIK_G				*/
//	, IKEY_H					/*DIK_H				*/
//	, IKEY_J					/*DIK_J				*/
//	, IKEY_K					/*DIK_K				*/
//	, IKEY_L					/*DIK_L				*/
//	, IKEY_SemiColon			/*DIK_SEMICOLON		*/
//	, IKEY_Apostrophe			/*DIK_APOSTROPHE	*/
//	, IKEY_Tilde				/*DIK_GRAVE			*/
//	, IKEY_LeftShift			/*DIK_LSHIFT		*/
//	, IKEY_BackSlash			/*DIK_BACKSLASH		*/
//	, IKEY_Z					/*DIK_Z				*/
//	, IKEY_X					/*DIK_X				*/
//	, IKEY_C					/*DIK_C				*/
//	, IKEY_V					/*DIK_V				*/
//	, IKEY_B					/*DIK_B				*/
//	, IKEY_N					/*DIK_N				*/
//	, IKEY_M					/*DIK_M				*/
//	, IKEY_Comma				/*DIK_COMMA			*/
//	, IKEY_Period				/*DIK_PERIOD		*/
//	, IKEY_ForwardSlash			/*DIK_SLASH			*/
//	, IKEY_RightShift			/*DIK_RSHIFT		*/
//	, IKEY_PAD_Asterisk			/*DIK_MULTIPLY		*/
//	, IKEY_NONE					/*DIK_LMENU			*/
//	, IKEY_Space				/*DIK_SPACE			*/
//	, IKEY_CapsLock				/*DIK_CAPITAL		*/
//	, IKEY_F1					/*DIK_F1			*/
//	, IKEY_F2					/*DIK_F2			*/
//	, IKEY_F3					/*DIK_F3			*/
//	, IKEY_F4					/*DIK_F4			*/
//	, IKEY_F5					/*DIK_F5			*/
//	, IKEY_F6					/*DIK_F6			*/
//	, IKEY_F7					/*DIK_F7			*/
//	, IKEY_F8					/*DIK_F8			*/
//	, IKEY_F9					/*DIK_F9			*/
//	, IKEY_F10					/*DIK_F10			*/
//	, IKEY_NumLock				/*DIK_NUMLOCK		*/
//	, IKEY_ScrollLock			/*DIK_SCROLL		*/
//	, IKEY_PAD_7				/*DIK_NUMPAD7		*/
//	, IKEY_PAD_8				/*DIK_NUMPAD8		*/
//	, IKEY_PAD_9				/*DIK_NUMPAD9		*/
//	, IKEY_PAD_Minus			/*DIK_SUBTRACT		*/
//	, IKEY_PAD_4				/*DIK_NUMPAD4		*/
//	, IKEY_PAD_5				/*DIK_NUMPAD5		*/
//	, IKEY_PAD_6				/*DIK_NUMPAD6		*/
//	, IKEY_PAD_Plus				/*DIK_ADD			*/
//	, IKEY_PAD_1				/*DIK_NUMPAD1		*/
//	, IKEY_PAD_2				/*DIK_NUMPAD2		*/
//	, IKEY_PAD_3				/*DIK_NUMPAD3		*/
//	, IKEY_PAD_0				/*DIK_NUMPAD0		*/
//	, IKEY_PAD_Period			/*DIK_DECIMAL		*/
//	, IKEY_NONE					/*DIK_OEM_102		*/
//	, IKEY_F11					/*DIK_F11			*/
//	, IKEY_F12					/*DIK_F12			*/
//	, IKEY_NONE					/*DIK_F13			*/
//	, IKEY_NONE					/*DIK_F14			*/
//	, IKEY_NONE					/*DIK_F15			*/
//	, IKEY_NONE					/*DIK_KANA			*/
//	, IKEY_NONE					/*DIK_ABNT_C1		*/
//	, IKEY_NONE					/*DIK_CONVERT		*/
//	, IKEY_NONE					/*DIK_NOCONVERT		*/
//	, IKEY_NONE					/*DIK_YEN			*/
//	, IKEY_NONE					/*DIK_ABNT_C2		*/
//	, IKEY_NONE					/*DIK_NUMPADEQUALS	*/
//	, IKEY_NONE					/*DIK_PREVTRACK		*/
//	, IKEY_NONE					/*DIK_AT			*/
//	, IKEY_NONE					/*DIK_COLON			*/
//	, IKEY_NONE					/*DIK_UNDERLINE		*/
//	, IKEY_NONE					/*DIK_KANJI			*/
//	, IKEY_NONE					/*DIK_STOP			*/
//	, IKEY_NONE					/*DIK_AX			*/
//	, IKEY_NONE					/*DIK_UNLABELED		*/
//	, IKEY_NONE					/*DIK_NEXTTRACK		*/
//	, IKEY_PAD_Enter			/*DIK_NUMPADENTER	*/
//	, IKEY_RightCtrl			/*DIK_RCONTROL		*/
//	, IKEY_NONE					/*DIK_MUTE			*/
//	, IKEY_NONE					/*DIK_CALCULATOR	*/
//	, IKEY_NONE					/*DIK_PLAYPAUSE		*/
//	, IKEY_NONE					/*DIK_MEDIASTOP		*/
//	, IKEY_NONE					/*DIK_VOLUMEDOWN	*/
//	, IKEY_NONE					/*DIK_VOLUMEUP		*/
//	, IKEY_NONE					/*DIK_WEBHOME		*/
//	, IKEY_NONE					/*DIK_NUMPADCOMMA	*/
//	, IKEY_PAD_ForwardSlash		/*DIK_DIVIDE		*/
//	, IKEY_NONE					/*DIK_SYSRQ			*/
//	, IKEY_NONE					/*DIK_RMENU			*/
//	, IKEY_NONE					/*DIK_PAUSE			*/
//	, IKEY_Home					/*DIK_HOME			*/
//	, IKEY_UpArrow				/*DIK_UP			*/
//	, IKEY_NONE					/*DIK_PRIOR			*/
//	, IKEY_LeftArrow			/*DIK_LEFT			*/
//	, IKEY_RightArrow			/*DIK_RIGHT			*/
//	, IKEY_End					/*DIK_END			*/
//	, IKEY_DownArrow			/*DIK_DOWN			*/
//	, IKEY_NONE					/*DIK_NEXT			*/
//	, IKEY_Insert				/*DIK_INSERT		*/
//	, IKEY_Delete				/*DIK_DELETE		*/
//	, IKEY_NONE					/*DIK_LWIN			*/
//	, IKEY_NONE					/*DIK_RWIN			*/
//	, IKEY_NONE					/*DIK_APPS			*/
//	, IKEY_NONE					/*DIK_POWER			*/
//	, IKEY_NONE					/*DIK_SLEEP			*/
//	, IKEY_NONE					/*DIK_WAKE			*/
//	, IKEY_NONE					/*DIK_WEBSEARCH		*/
//	, IKEY_NONE					/*DIK_WEBFAVORITES	*/
//	, IKEY_NONE					/*DIK_WEBREFRESH	*/
//	, IKEY_NONE					/*DIK_WEBSTOP		*/
//	, IKEY_NONE					/*DIK_WEBFORWARD	*/
//	, IKEY_NONE					/*DIK_WEBBACK		*/
//	, IKEY_NONE					/*DIK_MYCOMPUTER	*/
//	, IKEY_NONE					/*DIK_MAIL			*/
//	, IKEY_NONE					/*DIK_MEDIASELECT	*/
//};


InputHandler::InputHandler() {
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
}


InputHandler::~InputHandler() {
}

bool InputHandler::Startup(GLFWwindow* a_InputWindow) {
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HWND hwnd = glfwGetWin32Window(a_InputWindow);
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

	_CInput = this;

	return true;
}

void InputHandler::Shutdown() {
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

bool InputHandler::IsKeyDown(IKeys aKey) {
	return mKeyboardState[mKeyboardRemapper[aKey]];
}

bool InputHandler::WasKeyPressed(IKeys aKey) {
	const int index = mKeyboardRemapper[aKey];
	return mKeyboardState[index] && !mKeyboardStateOld[index];
}

glm::vec2 InputHandler::GetMousePos() {
	return glm::vec2(mMouseX, mMouseY);
}

glm::vec2 InputHandler::GetMouseDelta() {
	return glm::vec2(mMouseDeltaX, mMouseDeltaY);
}

bool InputHandler::IsMouseKeyDown(IMouseKeys aKey) {
	return mMouseState.rgbButtons[aKey] & 0x80;
}

bool InputHandler::WasMouseKeyPressed(IMouseKeys aKey) {
	return (mMouseState.rgbButtons[aKey] & 0x80) && !(mMouseStateOld.rgbButtons[aKey] & 0x80);
}

float InputHandler::GetMouseScroll() {
	return (float)mMouseState.lZ;
}

std::string InputHandler::GetKeysDown() {
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
}

bool InputHandler::ReadMouse() {
	HRESULT hr;


	//POINT pos;
	//if (::GetCursorPos(&pos) && ::ScreenToClient(Win32Application::GetHwnd(), &pos)) {
	//	mMouseDeltaX = m_MouseX - pos.x;
	//	mMouseDeltaY = m_MouseY - pos.y;
	//	m_MouseX = (float)pos.x;
	//	m_MouseY = (float)pos.y;
	//}

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
}
