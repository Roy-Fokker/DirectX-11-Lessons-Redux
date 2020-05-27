#pragma once

#include <Windows.h>
#include <memory>
#include <vector>
#include <future>

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

	class model_loading
	{
	public:
		model_loading() = delete;
		model_loading(HWND hWnd);
		~model_loading();

		auto on_resize(uintptr_t wParam, uintptr_t lParam) -> bool;
		auto exit() const -> bool;

		void update(const game_clock &clk, const raw_input &input);
		void render();

	private:
		void load_files();

		void create_pipeline_state_object();
		void make_default_ps();
		void make_text_ps();
		void make_sky_dome_ps();

		void create_mesh_buffers();
		void make_text_mesh();
		void make_sky_dome_mesh();
		
		void create_contant_buffers();
		void make_prespective_cb();
		void make_orthographic_cb();
		void make_view_cb();
		void make_text_transform_cb();
		
		void create_shader_resources();
		void make_text_texture();
		void make_sky_dome_texture();

		void input_update(const game_clock &clk, const raw_input &input);
		void camera_update();
		void text_update(const game_clock &clk);

		void draw_text();
		void draw_sky();

		void update_load_status();
		void draw_load_status();

	private:
		HWND hWnd;
		bool stop_drawing{ false };

		std::unique_ptr<direct3d11> d3d{};
		std::unique_ptr<direct2d1> d2d{};
		std::unique_ptr<render_pass> rp{};

		std::vector<std::unique_ptr<pipeline_state>> pipeline_states{};
		std::vector<std::unique_ptr<mesh_buffer>> mesh_buffers{};
		std::vector<std::unique_ptr<constant_buffer>> constant_buffers{};
		std::vector<std::unique_ptr<shader_resource>> shader_resources{};

		std::unique_ptr<camera> fp_cam{};
		
		std::vector<std::future<bool>> object_futures;
		std::vector<std::vector<uint8_t>> files_loaded;
	};
}