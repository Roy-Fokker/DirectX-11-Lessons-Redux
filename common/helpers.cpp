#include "helpers.h"

#include <fstream>
#include <cassert>

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

auto dx11_lessons::load_binary_file(const std::filesystem::path &path) -> std::vector<uint8_t>
{
	auto buffer = std::vector<uint8_t>{};

	auto file = std::ifstream(path, std::ios::in | std::ios::binary);
	assert(file.is_open());
	file.unsetf(std::ios::skipws);

	file.seekg(0, std::ios::end);
	buffer.reserve(file.tellg());
	file.seekg(0, std::ios::beg);

	std::copy(std::istream_iterator<uint8_t>(file),
	          std::istream_iterator<uint8_t>(),
	          std::back_inserter(buffer));

	return buffer;
}
