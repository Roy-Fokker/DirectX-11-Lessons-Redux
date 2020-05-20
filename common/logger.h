#pragma once

#include <Windows.h>
#include <string_view>

namespace dx11_lessons
{
	class notepad_logger
	{
	public:
		notepad_logger();
		~notepad_logger();

		void log(const std::string_view &message);
		void log(const std::wstring_view &message);

	private:
		auto find_notepad() -> bool;

	private:
		HWND notepad_window,
		     notepad_edit_panel;
	};

}