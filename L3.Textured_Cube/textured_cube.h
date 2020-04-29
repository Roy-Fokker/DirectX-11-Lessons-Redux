#pragma once

#include <Windows.h>
#include <memory>

namespace dx11_lessons
{
	class game_clock;
	class direct3d11;
	class render_pass;
	class pipeline_state;
	class mesh_buffer;
	class constant_buffer;

	class textured_cube
	{
	public:
		textured_cube() = delete;
		textured_cube(HWND hWnd);
		~textured_cube();

		auto on_keypress(uintptr_t wParam, uintptr_t lParam) -> bool;
		auto on_resize(uintptr_t wParam, uintptr_t lParam) -> bool;

		auto exit() const -> bool;
		void update(const game_clock &clk);
		void render();

	private:
		void create_pipeline_state_object();
		void create_mesh_buffer();
		void create_contant_buffers(HWND hWnd);

	private:
		bool stop_drawing{ false };

		std::unique_ptr<direct3d11> dx{};
		std::unique_ptr<render_pass> rp{};
		std::unique_ptr<pipeline_state> ps{};

		std::unique_ptr<mesh_buffer> cube_mb{};
		std::unique_ptr<constant_buffer> projection_cb{};
		std::unique_ptr<constant_buffer> view_cb{};
		std::unique_ptr<constant_buffer> transform_cb{};
	};
}