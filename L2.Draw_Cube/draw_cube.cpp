#include "draw_cube.h"

#include "direct3d11.h"
#include "render_pass.h"

#include "clock.h"

#include <array>

using namespace dx11_lessons;

namespace
{
	constexpr auto enable_vSync{ true };
	constexpr auto clear_color = std::array{ 0.4f, 0.5f, 0.4f, 1.0f };
}

draw_cube::draw_cube(HWND hWnd)
{
	dx = std::make_unique<direct3d11>(hWnd);
	rp = std::make_unique<render_pass>(dx->get_device(), dx->get_swapchain());
}

draw_cube::~draw_cube() = default;

auto draw_cube::on_keypress(uintptr_t wParam, uintptr_t lParam) -> bool
{
	auto &key = wParam;
	switch (key)
	{
		case VK_ESCAPE:
			stop_drawing = true;
			break;
	}

	return true;
}

auto draw_cube::on_resize(uintptr_t wParam, uintptr_t lParam) -> bool
{
	rp.reset(nullptr);

	dx->resize();

	rp = std::make_unique<render_pass>(dx->get_device(), dx->get_swapchain());

	return true;
}

auto draw_cube::exit() const -> bool
{
	return stop_drawing;
}

void draw_cube::update(const game_clock &clk)
{}

void draw_cube::render()
{
	auto context = dx->get_context();
	rp->activate(context);
	rp->clear(context, clear_color);

	dx->present(enable_vSync);
}
