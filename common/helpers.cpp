#include "helpers.h"

#include <ShlObj.h>
#include <atlbase.h>

#include <iostream>
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

	buffer.shrink_to_fit();

	return buffer;
}

auto dx11_lessons::open_file_dialog(HWND hWnd) -> std::filesystem::path
{
	::CoInitialize(NULL);

	constexpr auto file_types = std::array<COMDLG_FILTERSPEC, 1>
	{
		{
			L"Wavefront (*.obj)", L"*.obj"
		},
	};

	auto file_name = std::filesystem::path{};
	auto file_dialog = CComPtr<IFileDialog>{};
	auto hr = ::CoCreateInstance(CLSID_FileOpenDialog,
	                             nullptr,
	                             CLSCTX_INPROC_SERVER,
	                             IID_PPV_ARGS(&file_dialog));
	assert(SUCCEEDED(hr));

	file_dialog->SetTitle(L"Select obj model to view");
	file_dialog->SetFileTypes(static_cast<uint32_t>(file_types.size()),
	                          file_types.data());

	hr = file_dialog->Show(hWnd);
	if (FAILED(hr))
	{
		return {};
	}

	auto shell_item = CComPtr<IShellItem>{};
	hr = file_dialog->GetResult(&shell_item);
	assert(SUCCEEDED(hr));

	auto file_name_ptr = PWSTR{};
	hr = shell_item->GetDisplayName(SIGDN_FILESYSPATH, &file_name_ptr);
	assert(SUCCEEDED(hr));

	file_name.assign(file_name_ptr);
	return file_name;
}

memory_stream::memory_stream(char const *base, size_t size) :
	memory_buffer_stream(base, size), 
	std::istream(static_cast<std::streambuf *>(this))
{ }

memory_buffer_stream::memory_buffer_stream(char const *base, size_t size)
{
	char *p(const_cast<char *>(base));
	this->setg(p, p, p + size);
}
