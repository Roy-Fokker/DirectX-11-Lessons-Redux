#pragma once

#include <Windows.h>
#include <array>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <iosfwd>
#include <locale>

namespace dx11_lessons
{
	auto get_window_size(HWND window_handle) -> const std::array<uint16_t, 2>;
	auto load_binary_file(const std::filesystem::path &path) -> std::vector<uint8_t>;

	struct memory_buffer_stream : std::streambuf
	{
		memory_buffer_stream(char const *base, size_t size);
	};

	struct memory_stream : virtual memory_buffer_stream, std::istream
	{
		memory_stream(char const *base, size_t size);
	};

	template<class T> [[nodiscard]]
	auto ltrim(const std::basic_string<T> &str) -> std::basic_string<T>
	{
		auto last_char_it = std::find_if(str.begin(), str.end(), [](T ch)
		{
			return !std::isspace<T>(ch, std::locale::classic());
		});

		return std::basic_string<T>(str.begin(), last_char_it);
	}

	template<class T> [[nodiscard]]
	auto rtrim(const std::basic_string<T> &str) -> std::basic_string<T>
	{
		auto first_char_it = std::find_if(str.rbegin(), str.rend(), [](T ch)
		{
			return !std::isspace<T>(ch, std::locale::classic());
		});

		return std::basic_string<T>(first_char_it.base(), str.end());
	}

	template <typename T> [[nodiscard]]
	auto trim(const std::basic_string<T> &str) -> std::basic_string<T>
	{
		return ltrim(rtrim(str));
	}
}