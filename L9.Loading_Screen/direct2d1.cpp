#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#include "direct2d1.h"

#include <cassert>

using namespace dx11_lessons;

direct2d1::direct2d1(dxgi_device_t dxgi_device)
{
	auto hr = D2D1CreateDevice(dxgi_device, nullptr, &device);
	assert(SUCCEEDED(hr));

	auto options = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	hr = device->CreateDeviceContext(options, &context);
	assert(SUCCEEDED(hr));

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
	                         __uuidof(IDWriteFactory),
	                         reinterpret_cast<IUnknown **>(&write_factory));
	assert(SUCCEEDED(hr));
}

direct2d1::~direct2d1() = default;

void direct2d1::begin_draw(dxgi_surface_t surface, const D2D1::ColorF &clear_color)
{
	auto sd = DXGI_SURFACE_DESC{};
	auto hr = surface->GetDesc(&sd);

	auto bp = D2D1_BITMAP_PROPERTIES1{};
	bp.pixelFormat.format = sd.Format;
	bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	bp.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;

	auto bitmap = CComPtr<ID2D1Bitmap1>{};
	hr = context->CreateBitmapFromDxgiSurface(surface,
	                                          bp, &bitmap);
	assert(SUCCEEDED(hr));
	context->SetTarget(bitmap);

	context->BeginDraw();
	context->Clear(clear_color);
}

void direct2d1::draw_text(const std::wstring_view &text, 
                          D2D_POINT_2F position, D2D_POINT_2F size, 
                          text_format_t format, solid_color_brush_t brush)
{
	auto text_layout = make_text_layout(text, size, format);
	context->DrawTextLayout(position, text_layout, brush);
}

void direct2d1::end()
{
	context->EndDraw();
}

auto direct2d1::make_text_format(const std::wstring_view &font_name, float font_size) -> text_format_t
{
	auto text_format = text_format_t{};
	auto hr = write_factory->CreateTextFormat(font_name.data(), nullptr, 
											  DWRITE_FONT_WEIGHT_NORMAL,
											  DWRITE_FONT_STYLE_NORMAL,
											  DWRITE_FONT_STRETCH_NORMAL,
											  font_size,
											  L"",
											  &text_format);
	assert(SUCCEEDED(hr));

	return text_format;
}

auto direct2d1::make_solid_color_brush(D2D1::ColorF color) -> solid_color_brush_t
{
	auto solid_color_brush = solid_color_brush_t{};
	auto hr = context->CreateSolidColorBrush(color, &solid_color_brush);
	assert(SUCCEEDED(hr));

	return solid_color_brush;
}

auto direct2d1::make_text_layout(const std::wstring_view &text, D2D_POINT_2F size, text_format_t format) -> text_layout_t
{
	auto text_layout = text_layout_t{};
	auto hr = write_factory->CreateTextLayout(text.data(), static_cast<uint32_t>(text.size()), 
	                                          format, size.x, size.y, 
	                                          &text_layout);
	assert(SUCCEEDED(hr));

	return text_layout;
}
