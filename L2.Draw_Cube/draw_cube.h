#pragma once

#include <Windows.h>
#include <memory>

namespace dx11_lessons
{
	class game_clock;
	class direct3d11;
	class render_pass;

	class draw_cube
	{
	public:
		draw_cube() = delete;
		draw_cube(HWND hWnd);
		~draw_cube();

		auto on_keypress(uintptr_t wParam, uintptr_t lParam) -> bool;
		auto on_resize(uintptr_t wParam, uintptr_t lParam) -> bool;

		auto exit() const -> bool;
		void update(const game_clock &clk);
		void render();

	private:
		bool stop_drawing{ false };

		std::unique_ptr<direct3d11> dx{};
		std::unique_ptr<render_pass> rp{};
	};
}