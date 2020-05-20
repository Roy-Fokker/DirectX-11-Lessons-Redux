#include "logger.h"

#include <string>

using namespace dx11_lessons;

notepad_logger::notepad_logger() :
	notepad_window{},
	notepad_edit_panel{}
{
	find_notepad();
}

notepad_logger::~notepad_logger() = default;

void notepad_logger::log(const std::string_view &text)
{
	auto wtext = std::wstring(text.begin(), text.end());
	log(wtext);
}

void notepad_logger::log(const std::wstring_view &text)
{
	if (not notepad_edit_panel and not find_notepad())
	{
		return;
	}

	::SendMessage(notepad_edit_panel, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(text.data()));
}

auto notepad_logger::find_notepad() -> bool
{
	notepad_window = ::FindWindow(NULL, L"Untitled - Notepad");
	if (not notepad_window)
	{
		notepad_window = ::FindWindow(NULL, L"*Untitled - Notepad");
	}

	if (notepad_window)
	{
		notepad_edit_panel = ::FindWindowEx(notepad_window, NULL, L"EDIT", NULL);
	}

	if (notepad_window and notepad_edit_panel)
	{
		return true;
	}

	return false;
}
