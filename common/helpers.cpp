#include "helpers.h"

using namespace dx11_lessons;


auto dx11_lessons::get_window_size(HWND window_handle) -> const std::array<uint16_t, 2>
{
	RECT rect{};
	GetClientRect(window_handle, &rect);

	return
	{
		static_cast<uint16_t>(rect.right - rect.left),
		static_cast<uint16_t>(rect.bottom - rect.top)
	};
}