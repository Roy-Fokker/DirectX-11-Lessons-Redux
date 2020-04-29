#include "draw_cube.h"

#include "direct3d11.h"
#include "render_pass.h"
#include "pipeline_state.h"
#include "gpu_buffers.h"
#include "gpu_datatypes.h"

#include "clock.h"
#include "helpers.h"

#include <DirectXMath.h>
#include <array>
#include <vector>
#include <cmath>

using namespace dx11_lessons;
using namespace DirectX;

namespace
{
	constexpr auto enable_vSync{ true };
	constexpr auto clear_color = std::array{ 0.4f, 0.5f, 0.4f, 1.0f };
	constexpr auto field_of_view = 60.0f;
	constexpr auto near_z = 0.1f;
	constexpr auto far_z = 100.0f;

	using bs = pipeline_state::blend_type;
	using ds = pipeline_state::depth_stencil_type;
	using rs = pipeline_state::rasterizer_type;
	using ss = pipeline_state::sampler_type;
	using ie = pipeline_state::input_element_type;
}

draw_cube::draw_cube(HWND hWnd)
{
	dx = std::make_unique<direct3d11>(hWnd);
	rp = std::make_unique<render_pass>(dx->get_device(), dx->get_swapchain());

	create_pipeline_state_object();
	create_mesh_buffer();
	create_contant_buffers(hWnd);
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
{
	auto context = dx->get_context();
	static auto angle_deg = 0.0;
	angle_deg += 90.0f * clk.get_delta_s();
	if (angle_deg >= 360.0f)
	{
		angle_deg -= 360.0f;
	}

	auto angle = XMConvertToRadians(static_cast<float>(angle_deg));
	auto cube_pos = matrix{ XMMatrixIdentity() };
	cube_pos.data = XMMatrixRotationY(angle);
	cube_pos.data = XMMatrixTranspose(cube_pos.data);

	transform_cb->update(context,
	                     sizeof(matrix),
	                     reinterpret_cast<const void *>(&cube_pos));
}

void draw_cube::render()
{
	auto context = dx->get_context();

	ps->activate(context);

	projection_cb->activate(context);
	view_cb->activate(context);
	transform_cb->activate(context);
	cube_mb->activate(context);

	rp->activate(context);
	rp->clear(context, clear_color);

	cube_mb->draw(context);

	dx->present(enable_vSync);
}

void draw_cube::create_pipeline_state_object()
{
	auto vso = load_binary_file(L"vertex_shader.cso"),
	     pso = load_binary_file(L"pixel_shader.cso");

	auto desc = pipeline_state::description
	{
		bs::opaque,
		ds::read_write,
		rs::cull_anti_clockwise,
		ss::anisotropic_clamp,
		vertex_elements,
		vso,
		pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};

	ps = std::make_unique<pipeline_state>(dx->get_device(), desc);
}

void draw_cube::create_mesh_buffer()
{
	auto cube_vertices = std::vector{
		vertex{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
		vertex{ { -1.0f, +1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		vertex{ { +1.0f, +1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
		vertex{ { +1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		vertex{ { -1.0f, -1.0f, +1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		vertex{ { -1.0f, +1.0f, +1.0f }, { 0.0f, 1.0f, 1.0f, 1.0f } },
		vertex{ { +1.0f, +1.0f, +1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
		vertex{ { +1.0f, -1.0f, +1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
	};

	auto cube_indicies = std::vector<uint32_t>{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 6, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7,
	};

	auto device = dx->get_device();
	cube_mb = std::make_unique<mesh_buffer>(device, mesh{ cube_vertices, cube_indicies });
}

void draw_cube::create_contant_buffers(HWND hWnd)
{
	using slot = constant_buffer::shader_slot;
	using stage = constant_buffer::shader_stage;
	auto device = dx->get_device();

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
		auto view = matrix{ XMMatrixIdentity() };
		auto eye = XMVectorSet(0.0f, 2.0f, 5.0f, 0.0f),
		     focus = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		     up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		view.data = XMMatrixLookAtLH(eye, focus, up);
		view.data = XMMatrixTranspose(view.data);
		view_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::view,
		                                            sizeof(matrix),
		                                            reinterpret_cast<const void *>(&view));
	}

	// Transform
	{
		auto cube_pos = matrix{ XMMatrixTranslation(0.0f, 0.0f, 0.0f) };
		cube_pos.data = XMMatrixTranspose(cube_pos.data);
		transform_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::transform,
		                                                 sizeof(matrix),
		                                                 reinterpret_cast<const void *>(&cube_pos));
	}
}
