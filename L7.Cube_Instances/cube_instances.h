#pragma once

#include <Windows.h>
#include <memory>

namespace dx11_lessons
{
	class game_clock;
	class raw_input;
	class camera;
	class direct3d11;
	class direct2d1;
	class render_pass;
	class pipeline_state;
	class mesh_buffer;
	class constant_buffer;
	class shader_resource;

	class cube_instances
	{
	public:
		cube_instances() = delete;
		cube_instances(HWND hWnd);
		~cube_instances();

		auto on_resize(uintptr_t wParam, uintptr_t lParam) -> bool;

		auto exit() const -> bool;
		void update(const game_clock &clk, const raw_input &input);
		void render();

	private:
		void create_pipeline_state_object();
		void create_mesh_buffers();
		void create_contant_buffers(HWND hWnd);
		void create_shader_resources();

		void input_update(const game_clock &clk, const raw_input &input);

	private:
		bool stop_drawing{ false };

		std::unique_ptr<direct3d11> d3d{};
		std::unique_ptr<direct2d1> d2d{};
		std::unique_ptr<render_pass> rp{};

		std::unique_ptr<pipeline_state> ps{};
		std::unique_ptr<pipeline_state> light_ps{};

		std::unique_ptr<mesh_buffer> cube_mb{};
		std::unique_ptr<mesh_buffer> text_mb{};

		std::unique_ptr<constant_buffer> projection_cb{};

		std::unique_ptr<camera> fp_cam{};
		std::unique_ptr<constant_buffer> view_cb{};

		std::unique_ptr<constant_buffer> cube_cb{};
		std::unique_ptr<constant_buffer> light_cb{};

		std::unique_ptr<constant_buffer> text_cb{};

		std::unique_ptr<shader_resource> cube_sr{};
		std::unique_ptr<shader_resource> text_sr{};
	};
}