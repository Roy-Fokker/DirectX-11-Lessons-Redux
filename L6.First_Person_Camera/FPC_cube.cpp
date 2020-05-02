#include "FPC_cube.h"

#include "direct3d11.h"
#include "direct2d1.h"
#include "render_pass.h"
#include "pipeline_state.h"
#include "gpu_buffers.h"
#include "gpu_datatypes.h"

#include "raw_input.h"
#include "clock.h"
#include "helpers.h"

#include <fmt/core.h>
#include <DirectXMath.h>
#include <array>
#include <vector>
#include <cmath>

using namespace dx11_lessons;
using namespace DirectX;

namespace
{
	constexpr auto enable_vSync{ true };
	constexpr auto clear_color = std::array{ 0.2f, 0.2f, 0.2f, 1.0f };
	constexpr auto field_of_view = 60.0f;
	constexpr auto near_z = 0.1f;
	constexpr auto far_z = 100.0f;

	using bs = pipeline_state::blend_type;
	using ds = pipeline_state::depth_stencil_type;
	using rs = pipeline_state::rasterizer_type;
	using ss = pipeline_state::sampler_type;
	using ie = pipeline_state::input_element_type;
}

fpc_cube::fpc_cube(HWND hWnd)
{
	d3d = std::make_unique<direct3d11>(hWnd);
	d2d = std::make_unique<direct2d1>(d3d->get_dxgi_device());
	rp = std::make_unique<render_pass>(d3d->get_device(), d3d->get_swapchain());

	create_pipeline_state_object();
	create_mesh_buffers();
	create_contant_buffers(hWnd);
	create_shader_resources();
}

fpc_cube::~fpc_cube() = default;

auto fpc_cube::on_resize(uintptr_t wParam, uintptr_t lParam) -> bool
{
	rp.reset(nullptr);

	d3d->resize();

	rp = std::make_unique<render_pass>(d3d->get_device(), d3d->get_swapchain());

	return true;
}

auto fpc_cube::exit() const -> bool
{
	return stop_drawing;
}

void fpc_cube::update(const game_clock &clk, const raw_input &input)
{
	input_update(clk, input);

	auto cube_angle_text = std::wstring{};
	// Rotate Cube
	{
		static auto angle_deg = 0.0;
		angle_deg += 90.0f * clk.get_delta_s();
		if (angle_deg >= 360.0f)
		{
			angle_deg -= 360.0f;
		}
		cube_angle_text = fmt::format(L"Angle: {:06.2f}", angle_deg);

		auto angle = XMConvertToRadians(static_cast<float>(angle_deg));
		auto cube_pos = matrix{ XMMatrixIdentity() };
		cube_pos.data = XMMatrixRotationY(angle);
		cube_pos.data = XMMatrixTranspose(cube_pos.data);

		cube_cb->update(d3d->get_context(),
		                sizeof(matrix),
		                reinterpret_cast<const void *>(&cube_pos));
	}

	// Update Text
	{
		static auto fps = 0.0;
		static long frame_count = 0;
		static auto total_time = 0.0;
		static auto fps_text = std::wstring{};

		total_time += clk.get_delta_s();
		frame_count++;
		if (total_time > 1.0)
		{
			fps = frame_count / total_time;
			frame_count = 0;
			total_time = 0.0;

			fps_text = fmt::format(L"FPS: {:.2f}", fps);
		}

		auto format = d2d->make_text_format(L"Consolas", 75.0f);
		auto brush = d2d->make_solid_color_brush(D2D1::ColorF(D2D1::ColorF::Black));

		d2d->begin_draw(text_sr->get_dxgi_surface());
		d2d->draw_text(fps_text, { 10, 10 }, { 512, 512 }, format, brush);
		d2d->draw_text(cube_angle_text, { 10, 250 }, { 512, 512 }, format, brush);
		d2d->end();
	}
}

void fpc_cube::render()
{
	auto context = d3d->get_context();

	rp->activate(context);
	rp->clear(context, clear_color);

	projection_cb->activate(context);
	view_cb->activate(context);

	light_ps->activate(context);
	light_cb->activate(context);
	cube_cb->activate(context);
	cube_sr->activate(context);
	cube_mb->activate(context);
	cube_mb->draw(context);

	ps->activate(context);
	text_cb->activate(context);
	text_sr->activate(context);
	text_mb->activate(context);
	text_mb->draw(context);

	d3d->present(enable_vSync);
}

void fpc_cube::create_pipeline_state_object()
{
	auto vso = load_binary_file(L"vertex_shader.cso"),
	     basic_pso = load_binary_file(L"pixel_shader.cso"),
	     light_pso = load_binary_file(L"lighting.ps.cso");

	auto desc = pipeline_state::description
	{
		bs::opaque,
		ds::read_write,
		rs::cull_anti_clockwise,
		ss::anisotropic_clamp,
		vertex_elements,
		vso,
		basic_pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};

	ps = std::make_unique<pipeline_state>(d3d->get_device(), desc);

	auto light_desc = pipeline_state::description
	{
		bs::opaque,
		ds::read_write,
		rs::cull_anti_clockwise,
		ss::anisotropic_clamp,
		vertex_elements,
		vso,
		light_pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};

	light_ps = std::make_unique<pipeline_state>(d3d->get_device(), light_desc);
}

void fpc_cube::create_mesh_buffers()
{
	auto device = d3d->get_device();

	// Box mesh
	{
		auto l = 1.0f,
		     w = 1.0f,
		     h = 1.0f;

		auto front_normal = XMFLOAT3{  0.0f, 0.0f, 1.0f }, back_normal   = XMFLOAT3{ 0.0f,  0.0f, -1.0f },
		     left_normal  = XMFLOAT3{ -1.0f, 0.0f, 0.0f }, right_normal  = XMFLOAT3{ 1.0f,  0.0f,  0.0f },
		     top_normal   = XMFLOAT3{  0.0f, 1.0f, 0.0f }, bottom_normal = XMFLOAT3{ 0.0f, -1.0f,  0.0f };

		auto cube_mesh = mesh{
			// Vertex List
			{
				// Front
				{ { -l, -w, +h }, front_normal, { 0.0f, 0.0f } },
				{ { +l, -w, +h }, front_normal, { 1.0f, 0.0f } },
				{ { +l, +w, +h }, front_normal, { 1.0f, 1.0f } },
				{ { -l, +w, +h }, front_normal, { 0.0f, 1.0f } },

				// Bottom
				{ { -l, -w, -h }, bottom_normal, { 0.0f, 0.0f } },
				{ { +l, -w, -h }, bottom_normal, { 1.0f, 0.0f } },
				{ { +l, -w, +h }, bottom_normal, { 1.0f, 1.0f } },
				{ { -l, -w, +h }, bottom_normal, { 0.0f, 1.0f } },

				// Right
				{ { +l, -w, -h }, right_normal, { 0.0f, 0.0f } },
				{ { +l, +w, -h }, right_normal, { 1.0f, 0.0f } },
				{ { +l, +w, +h }, right_normal, { 1.0f, 1.0f } },
				{ { +l, -w, +h }, right_normal, { 0.0f, 1.0f } },

				// Left
				{ { -l, -w, -h }, left_normal, { 0.0f, 0.0f } },
				{ { -l, -w, +h }, left_normal, { 1.0f, 0.0f } },
				{ { -l, +w, +h }, left_normal, { 1.0f, 1.0f } },
				{ { -l, +w, -h }, left_normal, { 0.0f, 1.0f } },

				// Back
				{ { -l, -w, -h }, back_normal, { 0.0f, 0.0f } },
				{ { -l, +w, -h }, back_normal, { 1.0f, 0.0f } },
				{ { +l, +w, -h }, back_normal, { 1.0f, 1.0f } },
				{ { +l, -w, -h }, back_normal, { 0.0f, 1.0f } },

				// Top
				{ { -l, +w, -h }, top_normal, { 0.0f, 0.0f } },
				{ { -l, +w, +h }, top_normal, { 1.0f, 0.0f } },
				{ { +l, +w, +h }, top_normal, { 1.0f, 1.0f } },
				{ { +l, +w, -h }, top_normal, { 0.0f, 1.0f } },
			},


			// Index List
			{
				// Front
				0, 1, 2, 0, 2, 3,
				// Bottom
				4, 5, 6, 4, 6, 7,
				// Right
				8, 9, 10, 8, 10, 11,
				// Left
				12, 13, 14, 12, 14, 15,
				// Back
				16, 17, 18, 16, 18, 19,
				// Top
				20, 21, 22, 20, 22, 23,
			}
		};

		cube_mb = std::make_unique<mesh_buffer>(device, cube_mesh);
	}

	// Rectangle mesh
	{
		auto l = 1.0f,
		     w = 1.0f;
		auto rect_mesh = mesh{
			// Vertex List
			{
				{ { -l, +w, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ { +l, +w, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
				{ { +l, -w, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
				{ { -l, -w, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
			},

			// Index List
			{
				0, 1, 2,
				0, 2, 3
			}
		};

		text_mb = std::make_unique<mesh_buffer>(device, rect_mesh);
	}
}

void fpc_cube::create_contant_buffers(HWND hWnd)
{
	using slot = shader_slot;
	using stage = shader_stage;
	auto device = d3d->get_device();

	// Projection
	{
		auto [width, height] = get_window_size(hWnd);
		auto aspect_ratio = width / static_cast<float>(height);
		auto h_fov = XMConvertToRadians(field_of_view);
		auto v_fov = 2.0f * std::atan(std::tan(h_fov / 2.0f) * aspect_ratio);

		auto projection = matrix{ XMMatrixIdentity() };
		projection.data = XMMatrixPerspectiveFovLH(v_fov, aspect_ratio, near_z, far_z);
		projection.data = XMMatrixTranspose(projection.data);
		projection_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::projection,
		                                                  sizeof(matrix),
		                                                  reinterpret_cast<const void *>(&projection));
	}

	// View
	{
		auto eye = XMVectorSet(0.0f, 2.0f, 5.0f, 0.0f),
		     focus = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		     up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		auto view = camera{ XMMatrixIdentity() };
		view.matrix = XMMatrixLookAtLH(eye, focus, up);
		view.matrix = XMMatrixTranspose(view.matrix);
		XMStoreFloat3(&view.position, eye);
		view_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::view,
		                                            sizeof(matrix),
		                                            reinterpret_cast<const void *>(&view));
	}

	// Cube Transform
	{
		auto cube_pos = matrix{ XMMatrixTranslation(0.0f, 0.0f, 0.0f) };
		cube_pos.data = XMMatrixTranspose(cube_pos.data);
		cube_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::transform,
		                                            sizeof(matrix),
		                                            reinterpret_cast<const void *>(&cube_pos));
	}

	// Text Transform
	{
		auto text_pos = matrix{ XMMatrixTranslation(3.0f, 0.0f, 0.0f) };
		text_pos.data = XMMatrixRotationY(XMConvertToRadians(180.f)) * text_pos.data;
		text_pos.data = XMMatrixTranspose(text_pos.data);
		text_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::transform,
		                                            sizeof(matrix),
		                                            reinterpret_cast<const void *>(&text_pos));
	}

	// Light
	{
		auto light_data = light{};
		light_data.diffuse = { 0.0f, 0.0f, 1.0f, 1.0f };
		light_data.ambient = { 0.0f, 1.0f, 0.0f, 1.0f };
		light_data.light_dir = { 0.0f, -3.0f, 3.0f };
		light_data.specular_power = 1.0f;
		light_data.specular = { 1.0f, 0.0f, 0.0f, 1.0f };
		light_cb = std::make_unique<constant_buffer>(device, stage::pixel, slot::light,
		                                             sizeof(light),
		                                             reinterpret_cast<const void *>(&light_data));
	}
}

void fpc_cube::create_shader_resources()
{
	auto device = d3d->get_device();

	// Load Texture file
	{
		auto tex = load_binary_file(L"uv_grid.dds");
		cube_sr = std::make_unique<shader_resource>(device,
		                                            shader_stage::pixel, shader_slot::texture,
		                                            tex);
	}
	
	// Create Texture for Direct 2D and Direct Write
	{
		auto td = D3D11_TEXTURE2D_DESC{};
		td.Width = 512;
		td.Height = 512;
		td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc = { 1, 0 };
		td.ArraySize = 1;
		td.MipLevels = 1;

		auto text_tex = CComPtr<ID3D11Texture2D>{};
		auto hr = device->CreateTexture2D(&td, nullptr, &text_tex);
		assert(SUCCEEDED(hr));

		text_sr = std::make_unique<shader_resource>(device,
		                                            shader_stage::pixel, shader_slot::texture,
		                                            text_tex);
	}
}

void fpc_cube::input_update(const game_clock &clk, const raw_input &input)
{
	if (input.is_button_down(input_button::escape))
	{
		stop_drawing = true;
	}
}
