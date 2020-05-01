#pragma once

#include <d2d1_3.h>
#include <dwrite_3.h>
#include <atlbase.h>
#include <string_view>

namespace dx11_lessons
{
	class direct2d1
	{
	public:
		using dxgi_device_t = CComPtr<IDXGIDevice>;
		using device_t = CComPtr<ID2D1Device>;
		using context_t = CComPtr<ID2D1DeviceContext>;
		using dxgi_surface_t = CComPtr<IDXGISurface1>;

		using write_factory_t = CComPtr<IDWriteFactory>;
		using text_format_t = CComPtr<IDWriteTextFormat>;
		using text_layout_t = CComPtr<IDWriteTextLayout>;
		using solid_color_brush_t = CComPtr<ID2D1SolidColorBrush>;

	public:
		direct2d1() = delete;
		direct2d1(dxgi_device_t dxgi_device);
		~direct2d1();

		void begin_draw(dxgi_surface_t surface);
		void draw_text(const std::wstring_view &text, D2D_POINT_2F position, D2D_POINT_2F size, text_format_t format, solid_color_brush_t brush);
		void end();

		auto make_text_format(const std::wstring_view &font_name, float font_size)->text_format_t;
		auto make_solid_color_brush(D2D1::ColorF color) -> solid_color_brush_t;

	private:
		auto make_text_layout(const std::wstring_view &text, D2D_POINT_2F size, text_format_t format)->text_layout_t;

	private:
		device_t device;
		context_t context;
		write_factory_t write_factory;
	};
}
