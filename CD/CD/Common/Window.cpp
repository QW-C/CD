#include <CD/Common/Window.hpp>

namespace CD {

Window::Window(DisplayMode& properties, std::wstring title) :
	display_mode(properties),
	title(std::move(title)),
	closed_flag(false) {
	WNDCLASSEX wc {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = handle_messages;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 5);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = wcp;
	wc.hIconSm = ::LoadIcon(NULL, IDI_APPLICATION);

	::RegisterClassEx(&wc);
	handle = ::CreateWindowEx(NULL, wcp, title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, properties.width, properties.height, NULL, NULL, wc.hInstance, NULL);
	::SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)this);
	::ShowWindow(handle, SW_SHOW);
	::UpdateWindow(handle);

	resize_func = [](float, float) {};
}

Window::~Window() {
	::DestroyWindow(handle);
}

void Window::close() {
	closed_flag = true;
}

void Window::set_cursor(bool enable) {
	::ShowCursor(enable);
	mouse_keyboard.cursor_enable = enable;
}

void Window::poll_messages() {
	mouse_keyboard.d_wheel = 0;
	if(!mouse_keyboard.cursor_enable) {
		::SetCursorPos(display_mode.width / 2, display_mode.height / 2);
		mouse_keyboard.cursor_pos = {display_mode.width / 2.f, display_mode.height / 2.f};
	}

	for(MSG msg; ::PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE);) {
		if(msg.message == WM_QUIT) {
			closed_flag = true;
			return;
		}
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

LRESULT CALLBACK Window::handle_messages(HWND handle, UINT msg, WPARAM wparam, LPARAM lparam) {
	Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(handle, GWLP_USERDATA));
	switch(msg) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		window->mouse_keyboard.keys[wparam] = KeyState::Pressed;
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		window->mouse_keyboard.keys[wparam] = KeyState::Released;
		break;
	case WM_LBUTTONDOWN:
		window->mouse_keyboard.buttons[Button::MB1] = KeyState::Pressed;
		break;
	case WM_RBUTTONDOWN:
		window->mouse_keyboard.buttons[Button::MB2] = KeyState::Pressed;
		break;
	case WM_MBUTTONDOWN:
		window->mouse_keyboard.buttons[Button::MB3] = KeyState::Pressed;
		break;
	case WM_XBUTTONDOWN:
		if(GET_XBUTTON_WPARAM(wparam) == XBUTTON1) {
			window->mouse_keyboard.buttons[Button::MB4] = KeyState::Pressed;
		}
		else {
			window->mouse_keyboard.buttons[Button::MB5] = KeyState::Pressed;
		}
		break;
	case WM_LBUTTONUP:
		window->mouse_keyboard.buttons[Button::MB1] = KeyState::Released;
		break;
	case WM_RBUTTONUP:
		window->mouse_keyboard.buttons[Button::MB2] = KeyState::Released;
		break;
	case WM_MBUTTONUP:
		window->mouse_keyboard.buttons[Button::MB3] = KeyState::Released;
		break;
	case WM_XBUTTONUP:
		if(GET_XBUTTON_WPARAM(wparam) == XBUTTON1) {
			window->mouse_keyboard.buttons[Button::MB4] = KeyState::Released;
		}
		else {
			window->mouse_keyboard.buttons[Button::MB5] = KeyState::Released;
		}
		break;
	case WM_MOUSEMOVE:
		window->mouse_keyboard.cursor_pos = {static_cast<float>(GET_X_LPARAM(lparam)), static_cast<float>(GET_Y_LPARAM(lparam))};
		break;
	case WM_MOUSEWHEEL:
		window->mouse_keyboard.d_wheel += GET_WHEEL_DELTA_WPARAM(wparam);
		break;
	case WM_SIZE:
		//window->resize_func(LOWORD(lparam), HIWORD(lparam));
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(handle, msg, wparam, lparam);
	}
	return 0;
}

}