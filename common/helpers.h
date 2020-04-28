#pragma once

#include <Windows.h>
#include <array>

namespace dx11_lessons
{
	auto get_window_size(HWND window_handle) -> const std::array<uint16_t, 2>;
}