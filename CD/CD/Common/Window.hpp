#pragma once

#include <CD/Common/Common.hpp>
#include <string>
#include <functional>
#include <Windows.h>
#include <Windowsx.h>

namespace CD {

enum Key : std::uint32_t {
	BACK = 0x08,
	TAB = 0x09,
	CLEAR = 0x0C,
	RETURN = 0x0D,
	SHIFT = 0x10,
	CTRL = 0x11,
	ALT = 0x12,
	PAUSE = 0x13,
	CAPITAL = 0x14,
	KANA = 0x15,
	KANJI = 0x19,
	ESCAPE = 0x1B,
	CONVERT = 0x1C,
	NONCONVERT = 0x1D,
	ACCEPT = 0x1E,
	MODECHANGE = 0x1F,
	SPACE = 0x20,
	PRIOR = 0x21,
	NEXT = 0x22,
	END = 0x23,
	HOME = 0x24,
	LEFT = 0x25,
	UP = 0x26,
	RIGHT = 0x27,
	DOWN = 0x28,
	SELECT = 0x29,
	PRINT = 0x2A,
	EXECUTE = 0x2B,
	SNAPSHOT = 0x2C,
	INSERT = 0x2D,
	DEL = 0x2E,
	HELP = 0x2F,
	K0 = '0',
	K1 = '1',
	K2 = '2',
	K3 = '3',
	K4 = '4',
	K5 = '5',
	K6 = '6',
	K7 = '7',
	K8 = '8',
	K9 = '9',
	A = 'A',
	B = 'B',
	C = 'C',
	D = 'D',
	E = 'E',
	F = 'F',
	G = 'G',
	H = 'H',
	I = 'I',
	J = 'J',
	K = 'K',
	L = 'L',
	M = 'M',
	N = 'N',
	O = 'O',
	P = 'P',
	Q = 'Q',
	R = 'R',
	S = 'S',
	T = 'T',
	U = 'U',
	V = 'V',
	W = 'W',
	X = 'X',
	Y = 'Y',
	Z = 'Z',
	APPS = 0x5D,
	NUMPAD0 = 0x60,
	NUMPAD1 = 0x61,
	NUMPAD2 = 0x62,
	NUMPAD3 = 0x63,
	NUMPAD4 = 0x64,
	NUMPAD5 = 0x65,
	NUMPAD6 = 0x66,
	NUMPAD7 = 0x67,
	NUMPAD8 = 0x68,
	NUMPAD9 = 0x69,
	MULTIPLY = 0x6A,
	ADD = 0x6B,
	SEPARATOR = 0x6C,
	SUBTRACT = 0x6D,
	DECIMAL = 0x6E,
	DIVIDE = 0x6F,
	F1 = 0x70,
	F2 = 0x71,
	F3 = 0x72,
	F4 = 0x73,
	F5 = 0x74,
	F6 = 0x75,
	F7 = 0x76,
	F8 = 0x77,
	F9 = 0x78,
	F10 = 0x79,
	F11 = 0x7A,
	F12 = 0x7B,
	NUMLOCK = 0x90,
	SCROLL = 0x91,
	MISC_1 = 0xBA,
	MISC_PLUS = 0xBB,
	MISC_COMMA = 0xBC,
	MISC_MINUS = 0xBD,
	MISC_PERIOD = 0xBE,
	MISC_2 = 0xBF,
	MISC_3 = 0xC0,
	MISC_4 = 0xDB,
	MISC_5 = 0xDC,
	MISC_6 = 0xDD,
	MISC_7 = 0xDE,
	MISC_8 = 0xDF,
	MISC_102 = 0xE2,
	PROCESSKEY = 0xE5,
	PACKET = 0xE7,
	ATTN = 0xF6,
	CRSEL = 0xF7,
	EXSEL = 0xF8,
	EREOF = 0xF9,
	PLAY = 0xFA,
	ZOOM = 0xFB,
	NONAME = 0xFC,
	PA1 = 0xFD,
	MISC_CLEAR = 0xFE,
	MaxKeys = 512
};

enum Button : std::uint8_t {
	MB1,
	MB2,
	MB3,
	MB4,
	MB5,
	MaxButtons
};

enum class KeyState {
	Released = 0,
	Pressed = 1
};

struct CursorPosition {
	float x;
	float y;
};

struct MouseKeyboardInput {
	KeyState keys[Key::MaxKeys] {};
	KeyState buttons[Button::MaxButtons] {};
	CursorPosition cursor_pos {};
	bool cursor_enable = true;
	int d_wheel = 0;
};

struct DisplayMode {
	std::uint32_t width;
	std::uint32_t height;
};

class Window {
public:
	Window(DisplayMode&, std::wstring title);
	Window(const Window&) = delete;
	Window& operator=(const Window) = delete;
	~Window();

	void close();

	void poll_messages();

	void* get_handle() const;
	const DisplayMode& get_display_mode() const;
	bool closed() const;

	void set_cursor(bool enable);
	KeyState key_state(Key) const;
	KeyState button_state(Button) const;
	const CursorPosition& cursor_position() const;

	template<typename Fun> void set_resize_callback(Fun&&);
private:
	HWND handle;
	DisplayMode display_mode;
	std::wstring title;
	bool closed_flag;
	MouseKeyboardInput mouse_keyboard;

	std::function<void(float, float)> resize_func;

	static constexpr wchar_t* wcp = L"CDWidnow";
	static LRESULT __stdcall handle_messages(HWND, UINT, WPARAM, LPARAM);
};

inline void* Window::get_handle() const {
	return handle;
}

inline const DisplayMode& Window::get_display_mode() const {
	return display_mode;
}

inline bool Window::closed() const {
	return closed_flag;
}

inline KeyState Window::key_state(Key key) const {
	return mouse_keyboard.keys[key];
}

inline KeyState Window::button_state(Button button) const {
	return mouse_keyboard.buttons[button];
}

inline const CursorPosition& Window::cursor_position() const {
	return mouse_keyboard.cursor_pos;
}

template<typename Fun>
inline void Window::set_resize_callback(Fun&& f) {
	resize_func = f;
}

}