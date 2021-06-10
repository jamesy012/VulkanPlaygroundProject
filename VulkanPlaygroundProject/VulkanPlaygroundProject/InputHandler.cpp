#include "stdafx.h"
#include "InputHandler.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

InputHandler* _CInput;

LPDIRECTINPUT8 m_Di;
LPDIRECTINPUTDEVICE8 m_Keyboard;
LPDIRECTINPUTDEVICE8 m_Mouse;

unsigned char m_KeyboardState[256];
unsigned char m_KeyboardStateOld[256];
DIMOUSESTATE m_mouseState;
DIMOUSESTATE m_mouseStateOld;
int m_MouseX, m_MouseY;
int m_MouseDeltaX, m_MouseDeltaY;

unsigned char m_KeyboardRemapper[256] = { -1 };
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
	m_KeyboardRemapper[IKEY_Escape] = DIK_ESCAPE;
	m_KeyboardRemapper[IKEY_1] = DIK_1;
	m_KeyboardRemapper[IKEY_2] = DIK_2;
	m_KeyboardRemapper[IKEY_3] = DIK_3;
	m_KeyboardRemapper[IKEY_4] = DIK_4;
	m_KeyboardRemapper[IKEY_5] = DIK_5;
	m_KeyboardRemapper[IKEY_6] = DIK_6;
	m_KeyboardRemapper[IKEY_7] = DIK_7;
	m_KeyboardRemapper[IKEY_8] = DIK_8;
	m_KeyboardRemapper[IKEY_9] = DIK_9;
	m_KeyboardRemapper[IKEY_0] = DIK_0;
	m_KeyboardRemapper[IKEY_Minus] = DIK_MINUS;
	m_KeyboardRemapper[IKEY_Equals] = DIK_EQUALS;
	m_KeyboardRemapper[IKEY_Backspace] = DIK_BACK;
	m_KeyboardRemapper[IKEY_Tab] = DIK_TAB;
	m_KeyboardRemapper[IKEY_Q] = DIK_Q;
	m_KeyboardRemapper[IKEY_W] = DIK_W;
	m_KeyboardRemapper[IKEY_E] = DIK_E;
	m_KeyboardRemapper[IKEY_R] = DIK_R;
	m_KeyboardRemapper[IKEY_T] = DIK_T;
	m_KeyboardRemapper[IKEY_Y] = DIK_Y;
	m_KeyboardRemapper[IKEY_U] = DIK_U;
	m_KeyboardRemapper[IKEY_I] = DIK_I;
	m_KeyboardRemapper[IKEY_O] = DIK_O;
	m_KeyboardRemapper[IKEY_P] = DIK_P;
	m_KeyboardRemapper[IKEY_OpenSquareBracket] = DIK_LBRACKET;
	m_KeyboardRemapper[IKEY_CloseSquareBracket] = DIK_RBRACKET;
	m_KeyboardRemapper[IKEY_Enter] = DIK_RETURN;
	m_KeyboardRemapper[IKEY_LeftCtrl] = DIK_LCONTROL;
	m_KeyboardRemapper[IKEY_A] = DIK_A;
	m_KeyboardRemapper[IKEY_S] = DIK_S;
	m_KeyboardRemapper[IKEY_D] = DIK_D;
	m_KeyboardRemapper[IKEY_F] = DIK_F;
	m_KeyboardRemapper[IKEY_G] = DIK_G;
	m_KeyboardRemapper[IKEY_H] = DIK_H;
	m_KeyboardRemapper[IKEY_J] = DIK_J;
	m_KeyboardRemapper[IKEY_K] = DIK_K;
	m_KeyboardRemapper[IKEY_L] = DIK_L;
	m_KeyboardRemapper[IKEY_SemiColon] = DIK_SEMICOLON;
	m_KeyboardRemapper[IKEY_Apostrophe] = DIK_APOSTROPHE;
	m_KeyboardRemapper[IKEY_Tilde] = DIK_GRAVE;
	m_KeyboardRemapper[IKEY_LeftShift] = DIK_LSHIFT;
	m_KeyboardRemapper[IKEY_BackSlash] = DIK_BACKSLASH;
	m_KeyboardRemapper[IKEY_Z] = DIK_Z;
	m_KeyboardRemapper[IKEY_X] = DIK_X;
	m_KeyboardRemapper[IKEY_C] = DIK_C;
	m_KeyboardRemapper[IKEY_V] = DIK_V;
	m_KeyboardRemapper[IKEY_B] = DIK_B;
	m_KeyboardRemapper[IKEY_N] = DIK_N;
	m_KeyboardRemapper[IKEY_M] = DIK_M;
	m_KeyboardRemapper[IKEY_Comma] = DIK_COMMA;
	m_KeyboardRemapper[IKEY_Period] = DIK_PERIOD;
	m_KeyboardRemapper[IKEY_ForwardSlash] = DIK_SLASH;
	m_KeyboardRemapper[IKEY_RightShift] = DIK_RSHIFT;
	m_KeyboardRemapper[IKEY_PAD_Asterisk] = DIK_MULTIPLY;
	m_KeyboardRemapper[IKEY_Space] = DIK_SPACE;
	m_KeyboardRemapper[IKEY_CapsLock] = DIK_CAPITAL;
	m_KeyboardRemapper[IKEY_F1] = DIK_F1;
	m_KeyboardRemapper[IKEY_F2] = DIK_F2;
	m_KeyboardRemapper[IKEY_F3] = DIK_F3;
	m_KeyboardRemapper[IKEY_F4] = DIK_F4;
	m_KeyboardRemapper[IKEY_F5] = DIK_F5;
	m_KeyboardRemapper[IKEY_F6] = DIK_F6;
	m_KeyboardRemapper[IKEY_F7] = DIK_F7;
	m_KeyboardRemapper[IKEY_F8] = DIK_F8;
	m_KeyboardRemapper[IKEY_F9] = DIK_F9;
	m_KeyboardRemapper[IKEY_F10] = DIK_F10;
	m_KeyboardRemapper[IKEY_NumLock] = DIK_NUMLOCK;
	m_KeyboardRemapper[IKEY_ScrollLock] = DIK_SCROLL;
	m_KeyboardRemapper[IKEY_PAD_7] = DIK_NUMPAD7;
	m_KeyboardRemapper[IKEY_PAD_8] = DIK_NUMPAD8;
	m_KeyboardRemapper[IKEY_PAD_9] = DIK_NUMPAD9;
	m_KeyboardRemapper[IKEY_PAD_Minus] = DIK_SUBTRACT;
	m_KeyboardRemapper[IKEY_PAD_4] = DIK_NUMPAD4;
	m_KeyboardRemapper[IKEY_PAD_5] = DIK_NUMPAD5;
	m_KeyboardRemapper[IKEY_PAD_6] = DIK_NUMPAD6;
	m_KeyboardRemapper[IKEY_PAD_Plus] = DIK_ADD;
	m_KeyboardRemapper[IKEY_PAD_1] = DIK_NUMPAD1;
	m_KeyboardRemapper[IKEY_PAD_2] = DIK_NUMPAD2;
	m_KeyboardRemapper[IKEY_PAD_3] = DIK_NUMPAD3;
	m_KeyboardRemapper[IKEY_PAD_0] = DIK_NUMPAD0;
	m_KeyboardRemapper[IKEY_PAD_Period] = DIK_DECIMAL;
	m_KeyboardRemapper[IKEY_F11] = DIK_F11;
	m_KeyboardRemapper[IKEY_F12] = DIK_F12;
	m_KeyboardRemapper[IKEY_PAD_Enter] = DIK_NUMPADENTER;
	m_KeyboardRemapper[IKEY_RightCtrl] = DIK_RCONTROL;
	m_KeyboardRemapper[IKEY_PAD_ForwardSlash] = DIK_DIVIDE;
	m_KeyboardRemapper[IKEY_Home] = DIK_HOME;
	m_KeyboardRemapper[IKEY_UpArrow] = DIK_UP;
	m_KeyboardRemapper[IKEY_LeftArrow] = DIK_LEFT;
	m_KeyboardRemapper[IKEY_RightArrow] = DIK_RIGHT;
	m_KeyboardRemapper[IKEY_End] = DIK_END;
	m_KeyboardRemapper[IKEY_DownArrow] = DIK_DOWN;
	m_KeyboardRemapper[IKEY_Insert] = DIK_INSERT;
	m_KeyboardRemapper[IKEY_Delete] = DIK_DELETE;
}


InputHandler::~InputHandler() {
}

bool InputHandler::Startup(GLFWwindow* a_InputWindow) {
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HWND hwnd = glfwGetWin32Window(a_InputWindow);
	HRESULT hr;

	hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&m_Di, NULL);
	if (FAILED(hr)) {
		return false;
	}

	hr = m_Di->CreateDevice(GUID_SysKeyboard, &m_Keyboard, NULL);
	if (FAILED(hr)) {
		return false;
	}

	hr = m_Keyboard->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr)) {
		return false;
	}

	hr = m_Keyboard->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);//DISCL_NONEXCLUSIVE | DISCL_BACKGROUND
	if (FAILED(hr)) {
		return false;
	}

	hr = m_Keyboard->Acquire();
	if (FAILED(hr)) {
		return false;
	}

	hr = m_Di->CreateDevice(GUID_SysMouse, &m_Mouse, NULL);
	if (FAILED(hr)) {
		return false;
	}

	hr = m_Mouse->SetDataFormat(&c_dfDIMouse);
	if (FAILED(hr)) {
		return false;
	}

	hr = m_Mouse->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);//DISCL_NONEXCLUSIVE | DISCL_BACKGROUND
	if (FAILED(hr)) {
		return false;
	}

	hr = m_Mouse->Acquire();
	if (FAILED(hr)) {
		return false;
	}

	_CInput = this;

	return true;
}

void InputHandler::Shutdown() {
	if (m_Mouse) {
		m_Mouse->Unacquire();
		m_Mouse->Release();
		m_Mouse = nullptr;
	}

	if (m_Keyboard) {
		m_Keyboard->Unacquire();
		m_Keyboard->Release();
		m_Keyboard = nullptr;
	}

	if (m_Di) {
		m_Di->Release();
		m_Di = nullptr;
	}
}

void InputHandler::Update() {

	ReadKeyboard();
	ReadMouse();

	//// Update the location of the mouse cursor based on the change of the mouse location during the frame.
	//m_MouseX += m_mouseState.lX;
	//m_MouseY += m_mouseState.lY;
	//
	//// Ensure the mouse location doesn't exceed the screen width or height.
	//if (m_MouseX < 0) { m_MouseX = 0; }
	//if (m_MouseY < 0) { m_MouseY = 0; }
	//
	//if (m_MouseX > m_MainWindow->m_WindowWidth) { m_MouseX = m_MainWindow->m_WindowWidth; }
	//if (m_MouseY > m_MainWindow->m_WindowHeight) { m_MouseY = m_MainWindow->m_WindowHeight; }

}

bool InputHandler::IsKeyDown(IKeys a_Key) {
	return m_KeyboardState[m_KeyboardRemapper[a_Key]];
}

bool InputHandler::WasKeyPressed(IKeys a_Key) {
	const int index = m_KeyboardRemapper[a_Key];
	return m_KeyboardState[index] && !m_KeyboardStateOld[index];
}

glm::vec2 InputHandler::GetMousePos() {
	return glm::vec2(m_MouseX, m_MouseY);
}

glm::vec2 InputHandler::GetMouseDelta() {
	return glm::vec2(m_MouseDeltaX, m_MouseDeltaY);
}

bool InputHandler::IsMouseKeyDown(IMouseKeys a_Key) {
	return m_mouseState.rgbButtons[a_Key] & 0x80;
}

bool InputHandler::WasMouseKeyPressed(IMouseKeys a_Key) {
	return (m_mouseState.rgbButtons[a_Key] & 0x80) && !(m_mouseStateOld.rgbButtons[a_Key] & 0x80);
}

float InputHandler::GetMouseScroll() {
	return m_mouseState.lZ;
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

	memcpy(m_KeyboardStateOld, m_KeyboardState, sizeof(m_KeyboardState));
	if (!m_GetNewValues) {
		memset(m_KeyboardState, 0, sizeof(m_KeyboardState));
		return false;
	}
	// Read the keyboard device.
	hr = m_Keyboard->GetDeviceState(sizeof(m_KeyboardState), (LPVOID)&m_KeyboardState);
	if (FAILED(hr)) {
		// If the keyboard lost focus or was not acquired then try to get control back.
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED)) {
			m_Keyboard->Acquire();
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
	//	m_MouseDeltaX = m_MouseX - pos.x;
	//	m_MouseDeltaY = m_MouseY - pos.y;
	//	m_MouseX = (float)pos.x;
	//	m_MouseY = (float)pos.y;
	//}

	if (m_Mouse == nullptr) {
		return false;
	}

	memcpy(&m_mouseStateOld, &m_mouseState, sizeof(m_mouseState));
	if (!m_GetNewValues) {
		memset(&m_mouseState, 0, sizeof(m_mouseState));
		return false;
	}
	// Read the mouse device.
	hr = m_Mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&m_mouseState);
	if (FAILED(hr)) {
		// If the mouse lost focus or was not acquired then try to get control back.
		if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED)) {
			m_Mouse->Acquire();
		} else {
			return false;
		}
	} else {
		m_MouseDeltaX = m_mouseState.lX;
		m_MouseDeltaY = m_mouseState.lY;
		m_MouseX + m_MouseDeltaX;
		m_MouseY + m_MouseDeltaY;
		//move m_MouseX and co to be here and use m_MouseState instead
	}
	return true;
}
