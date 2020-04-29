#pragma once

#include <Windows.h>
#include <array>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace dx11_lessons
{
	auto get_window_size(HWND window_handle) -> const std::array<uint16_t, 2>;
	auto load_binary_file(const std::filesystem::path &path) -> std::vector<uint8_t>;
}