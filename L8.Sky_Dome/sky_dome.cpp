﻿#include "sky_dome.h"

#include "direct3d11.h"
#include "direct2d1.h"
#include "render_pass.h"
#include "pipeline_state.h"
#include "gpu_buffers.h"
#include "gpu_datatypes.h"

#include "camera.h"
#include "raw_input.h"
#include "clock.h"
#include "helpers.h"

#include <cppitertools\enumerate.hpp>
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
	const auto d2d_clear_color = D2D1::ColorF(D2D1::ColorF::Black, 0.0f);
	constexpr auto field_of_view = 60.0f;
	constexpr auto near_z = 0.1f;
	constexpr auto far_z = 100.0f;

	constexpr auto front_normal = XMFLOAT3{ 0.0f, 0.0f, 1.0f }, back_normal = XMFLOAT3{ 0.0f,  0.0f, -1.0f };
	constexpr auto left_normal = XMFLOAT3{ -1.0f, 0.0f, 0.0f }, right_normal = XMFLOAT3{ 1.0f,  0.0f,  0.0f };
	constexpr auto top_normal = XMFLOAT3{ 0.0f, 1.0f, 0.0f }, bottom_normal = XMFLOAT3{ 0.0f, -1.0f,  0.0f };

	const auto cube_base = mesh{
		// Vertex List
		{
			// Front
			{ { -1.0f, -1.0f, +1.0f }, front_normal, { 0.0f, 0.0f } },
			{ { +1.0f, -1.0f, +1.0f }, front_normal, { 1.0f, 0.0f } },
			{ { +1.0f, +1.0f, +1.0f }, front_normal, { 1.0f, 1.0f } },
			{ { -1.0f, +1.0f, +1.0f }, front_normal, { 0.0f, 1.0f } },

			// Bottom
			{ { -1.0f, -1.0f, -1.0f }, bottom_normal, { 0.0f, 0.0f } },
			{ { +1.0f, -1.0f, -1.0f }, bottom_normal, { 1.0f, 0.0f } },
			{ { +1.0f, -1.0f, +1.0f }, bottom_normal, { 1.0f, 1.0f } },
			{ { -1.0f, -1.0f, +1.0f }, bottom_normal, { 0.0f, 1.0f } },

			// right
			{ { +1.0f, -1.0f, -1.0f }, right_normal, { 0.0f, 0.0f } },
			{ { +1.0f, +1.0f, -1.0f }, right_normal, { 1.0f, 0.0f } },
			{ { +1.0f, +1.0f, +1.0f }, right_normal, { 1.0f, 1.0f } },
			{ { +1.0f, -1.0f, +1.0f }, right_normal, { 0.0f, 1.0f } },

			// left
			{ { -1.0f, -1.0f, -1.0f }, left_normal, { 0.0f, 0.0f } },
			{ { -1.0f, -1.0f, +1.0f }, left_normal, { 1.0f, 0.0f } },
			{ { -1.0f, +1.0f, +1.0f }, left_normal, { 1.0f, 1.0f } },
			{ { -1.0f, +1.0f, -1.0f }, left_normal, { 0.0f, 1.0f } },

			// Back
			{ { -1.0f, -1.0f, -1.0f }, back_normal, { 0.0f, 0.0f } },
			{ { -1.0f, +1.0f, -1.0f }, back_normal, { 1.0f, 0.0f } },
			{ { +1.0f, +1.0f, -1.0f }, back_normal, { 1.0f, 1.0f } },
			{ { +1.0f, -1.0f, -1.0f }, back_normal, { 0.0f, 1.0f } },

			// Top
			{ { -1.0f, +1.0f, -1.0f }, top_normal, { 0.0f, 0.0f } },
			{ { -1.0f, +1.0f, +1.0f }, top_normal, { 1.0f, 0.0f } },
			{ { +1.0f, +1.0f, +1.0f }, top_normal, { 1.0f, 1.0f } },
			{ { +1.0f, +1.0f, -1.0f }, top_normal, { 0.0f, 1.0f } },
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

	using bs = pipeline_state::blend_type;
	using ds = pipeline_state::depth_stencil_type;
	using rs = pipeline_state::rasterizer_type;
	using ss = pipeline_state::sampler_type;
	using ie = pipeline_state::input_element_type;

	using slot = shader_slot;
	using stage = shader_stage;

	auto spherify_and_invert(const mesh &cube, uint32_t subdivide_count) -> mesh
	{
		auto divide_edge = [](vertex v0, vertex v1) -> vertex
		{
			auto mp = vertex{};
			mp.position = XMFLOAT3{
				0.5f * (v0.position.x + v1.position.x),
				0.5f * (v0.position.y + v1.position.y),
				0.5f * (v0.position.z + v1.position.z),
			};
			mp.normal = v0.normal;
			mp.texcoord = XMFLOAT2{
				0.5f * (v0.texcoord.x + v1.texcoord.x),
				0.5f * (v0.texcoord.y + v1.texcoord.y),
			};

			return mp;
		};

		auto sub_divide_triangle = [&](const vertex &v0, const vertex &v1, const vertex &v2, const uint32_t index) -> mesh
		{
			auto m0 = divide_edge(v0, v1),
				m1 = divide_edge(v1, v2),
				m2 = divide_edge(v2, v0);

			return mesh
			{
				{ v0, m0, v1, m1, v2, m2 },
				{
					index + 0, index + 1, index + 5,
					index + 1, index + 2, index + 3,
					index + 3, index + 4, index + 5,
					index + 1, index + 3, index + 5,
				}
			};
		};

		auto sub_divide_mesh = [&](const mesh &shape) -> mesh
		{
			auto result_mesh = mesh{};
			auto &vertices = result_mesh.vertices;
			auto &indicies = result_mesh.indicies;

			auto number_of_triangles = shape.indicies.size() / 3u;
			for (size_t i = 0; i < number_of_triangles; i++)
			{
				auto triangle = sub_divide_triangle(shape.vertices[shape.indicies[i * 3]],
				                                    shape.vertices[shape.indicies[i * 3 + 1]],
				                                    shape.vertices[shape.indicies[i * 3 + 2]],
				                                    static_cast<uint32_t>(i) * 6);

				vertices.insert(vertices.end(), triangle.vertices.begin(), triangle.vertices.end());
				indicies.insert(indicies.end(), triangle.indicies.begin(), triangle.indicies.end());
			}

			return result_mesh;
		};

		auto result_mesh = mesh{};

		for (uint16_t i = 0; i < subdivide_count; i++)
		{
			auto &msh = (i == 0) ? cube : result_mesh;

			result_mesh = sub_divide_mesh(msh);
		}

		for (auto &v : result_mesh.vertices)
		{
			auto n = XMLoadFloat3(&v.normal);
			n = -1.0f * n;
			XMStoreFloat3(&v.normal, n);

			auto p = XMLoadFloat3(&v.position);
			p = XMVector3Normalize(p);
			XMStoreFloat3(&v.position, p);
		}

		return result_mesh;
	}
}

sky_dome::sky_dome(HWND hwnd) :
	hWnd{ hwnd }
{
	d3d = std::make_unique<direct3d11>(hWnd);
	d2d = std::make_unique<direct2d1>(d3d->get_dxgi_device());
	rp = std::make_unique<render_pass>(d3d->get_device(), d3d->get_swapchain());

	create_pipeline_state_object();
	create_mesh_buffers();
	create_contant_buffers();
	create_shader_resources();
}

sky_dome::~sky_dome() = default;

auto sky_dome::on_resize(uintptr_t wParam, uintptr_t lParam) -> bool
{
	rp.reset(nullptr);

	d3d->resize();

	rp = std::make_unique<render_pass>(d3d->get_device(), d3d->get_swapchain());
	make_prespective_cb();
	make_orthographic_cb();
	make_text_texture();

	return true;
}

auto sky_dome::exit() const -> bool
{
	return stop_drawing;
}

void sky_dome::update(const game_clock &clk, const raw_input &input)
{
	input_update(clk, input);
	cube_update(clk);
	camera_update();
	cube_instance_update(clk);
	text_update(clk);
}

void sky_dome::render()
{
	auto context = d3d->get_context();

	rp->activate(context);
	rp->clear(context, clear_color);

	prespective_proj_cb->activate(context);
	view_cb->activate(context);

	light_ps->activate(context);
	light_cb->activate(context);
	cube_cb->activate(context);
	cube_sr->activate(context);
	cube_mb->activate(context);
	cube_mb->draw(context);

	cube_instance_ps->activate(context);
	cube_instance_mb->activate(context);
	cube_instance_mb->draw(context);

	sky_dome_ps->activate(context);
	sky_dome_sr->activate(context);
	sky_dome_mb->activate(context);
	sky_dome_mb->draw(context);

	orthographic_proj_cb->activate(context);
	screen_text_ps->activate(context);
	text_cb->activate(context);
	text_sr->activate(context);
	text_mb->activate(context);
	text_mb->draw(context);

	d3d->present(enable_vSync);
}

void sky_dome::create_pipeline_state_object()
{
	make_default_ps();
	make_light_ps();
	make_text_ps();
	make_cube_instance_ps();
	make_sky_dome_ps();
}

void sky_dome::make_default_ps()
{
	auto device = d3d->get_device();
	auto vso = load_binary_file(L"vertex_shader.cso"),
	     basic_pso = load_binary_file(L"pixel_shader.cso");

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
	ps = std::make_unique<pipeline_state>(device, desc);
}

void sky_dome::make_light_ps()
{
	auto device = d3d->get_device();
	auto vso = load_binary_file(L"vertex_shader.cso"),
	     light_pso = load_binary_file(L"lighting.ps.cso");

	auto light_desc = pipeline_state::description
	{
		bs::alpha,
		ds::read_write,
		rs::cull_anti_clockwise,
		ss::anisotropic_clamp,
		vertex_elements,
		vso,
		light_pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};

	light_ps = std::make_unique<pipeline_state>(device, light_desc);
}

void sky_dome::make_text_ps()
{
	auto device = d3d->get_device();
	auto screen_text_vso = load_binary_file(L"screen_space_text.vs.cso"),
	     basic_pso = load_binary_file(L"pixel_shader.cso");

	auto screen_text_desc = pipeline_state::description
	{
		bs::non_premultipled,
		ds::read_write,
		rs::cull_anti_clockwise,
		ss::anisotropic_clamp,
		vertex_elements,
		screen_text_vso,
		basic_pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	screen_text_ps = std::make_unique<pipeline_state>(device, screen_text_desc);
}

void sky_dome::make_cube_instance_ps()
{
	auto device = d3d->get_device();
	auto vso = load_binary_file(L"cube_instances.vs.cso"),
	     basic_pso = load_binary_file(L"pixel_shader.cso");

	auto desc = pipeline_state::description
	{
		bs::opaque,
		ds::read_write,
		rs::cull_anti_clockwise,
		ss::anisotropic_clamp,
		instanced_vertex_elements,
		vso,
		basic_pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	cube_instance_ps = std::make_unique<pipeline_state>(device, desc);
}

void sky_dome::make_sky_dome_ps()
{
	auto device = d3d->get_device();
	auto vso = load_binary_file(L"sky_dome.vs.cso"),
		pso = load_binary_file(L"sky_dome.ps.cso");

	auto sky_dome_desc = pipeline_state::description
	{
		bs::opaque,
		ds::read_write,
		rs::cull_clockwise,
		ss::anisotropic_clamp,
		vertex_elements,
		vso, pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};

	sky_dome_ps = std::make_unique<pipeline_state>(device, sky_dome_desc);
}

void sky_dome::create_mesh_buffers()
{
	make_cube_mesh();
	make_text_mesh();
	make_cube_instance_mesh();
	make_sky_dome_mesh();
}

void sky_dome::make_cube_mesh()
{
	auto device = d3d->get_device();
	auto cube_mesh = spherify_and_invert(cube_base, 2);
	cube_mb = std::make_unique<mesh_buffer>(device, cube_mesh);
}

void sky_dome::make_cube_instance_mesh()
{
	auto device = d3d->get_device();
	auto l = 1.0f,
		w = 1.0f,
		h = 1.0f;

	auto front_normal = XMFLOAT3{ 0.0f, 0.0f, 1.0f }, back_normal = XMFLOAT3{ 0.0f,  0.0f, -1.0f },
		left_normal = XMFLOAT3{ -1.0f, 0.0f, 0.0f }, right_normal = XMFLOAT3{ 1.0f,  0.0f,  0.0f },
		top_normal = XMFLOAT3{ 0.0f, 1.0f, 0.0f }, bottom_normal = XMFLOAT3{ 0.0f, -1.0f,  0.0f };

	auto cube_mesh = instanced_mesh{
		cube_base.vertices, 
		cube_base.indicies
	};

	cube_mesh.instance_transforms.resize(100);
	for (auto &&[idx, transform] : cube_mesh.instance_transforms | iter::enumerate)
	{
		auto i = static_cast<int>(idx);
		constexpr auto x_max = 10;
		float x = -15.0f + 3.0f * (i % x_max),
		      z = -30.0f + 3.0f * (i / x_max);

		transform.data = XMMatrixTranslation(x, 0.0f, z);
		transform.data = XMMatrixTranspose(transform.data);
	}

	cube_instance_mb = std::make_unique<mesh_buffer>(device, cube_mesh);
}

void sky_dome::make_text_mesh()
{
	auto device = d3d->get_device();
	auto [width, height] = get_window_size(hWnd);
	auto l = width / 2.0f,
	     w = height / 2.0f;
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

void sky_dome::make_sky_dome_mesh()
{
	auto device = d3d->get_device();
	auto dome = spherify_and_invert(cube_base, 4);
	sky_dome_mb = std::make_unique<mesh_buffer>(device, dome);
}

void sky_dome::create_contant_buffers()
{
	// Projection
	make_prespective_cb();
	make_orthographic_cb();
	
	// View
	make_view_cb();
	
	// Cube Transform
	make_cube_transform_cb();

	// Text Transform
	make_text_transform_cb();

	// Light
	make_light_data_cb();
}

void sky_dome::make_prespective_cb()
{
	auto device = d3d->get_device();
	auto [width, height] = get_window_size(hWnd);

	auto aspect_ratio = width / static_cast<float>(height);
	auto h_fov = XMConvertToRadians(field_of_view);
	auto v_fov = 2.0f * std::atan(std::tan(h_fov / 2.0f) * aspect_ratio);

	auto projection = matrix{};
	projection.data = XMMatrixPerspectiveFovLH(v_fov, aspect_ratio, near_z, far_z);
	projection.data = XMMatrixTranspose(projection.data);
	prespective_proj_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::projection, projection);
}

void sky_dome::make_orthographic_cb()
{
	auto device = d3d->get_device();
	auto [width, height] = get_window_size(hWnd);

	auto projection = matrix{};
	projection.data = XMMatrixOrthographicLH(width, height, -1.0f, 1.0f);
	projection.data = XMMatrixTranspose(projection.data);
	orthographic_proj_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::projection, projection);
}

void sky_dome::make_view_cb()
{
	auto device = d3d->get_device();

	fp_cam = std::make_unique<camera>(XMFLOAT3{ 0.0f, 2.0f, 5.0f },
	                                  XMFLOAT3{ 0.0f, 0.0f, 0.0f },
	                                  XMFLOAT3{ 0.0f, 1.0f, 0.0f });
	auto view = view_matrix{};
	view.matrix = XMMatrixTranspose(fp_cam->get_view());
	view.position = fp_cam->get_position();
	view_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::view, view);
}

void sky_dome::make_cube_transform_cb()
{
	auto device = d3d->get_device();

	auto cube_pos = matrix{ XMMatrixTranslation(0.0f, 1.0f, 0.0f) };
	cube_pos.data = XMMatrixTranspose(cube_pos.data);
	cube_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::transform, cube_pos);

}

void sky_dome::make_text_transform_cb()
{
	auto device = d3d->get_device();

	auto text_pos = matrix{ XMMatrixTranslation(0.0f, 0.0f, 0.0f) };
	text_pos.data = XMMatrixTranspose(text_pos.data);
	text_cb = std::make_unique<constant_buffer>(device, stage::vertex, slot::transform, text_pos);
}

void sky_dome::make_light_data_cb()
{
	auto device = d3d->get_device();

	auto light_data = light{};
	light_data.diffuse = { 0.0f, 0.0f, 1.0f, 1.0f };
	light_data.ambient = { 0.0f, 1.0f, 0.0f, 1.0f };
	light_data.light_pos = { 0.0f, 3.0f, 5.0f };
	light_data.specular_power = 32.0f;
	light_data.specular = { 1.0f, 0.0f, 0.0f, 1.0f };
	light_cb = std::make_unique<constant_buffer>(device, stage::pixel, slot::light, light_data);
}

void sky_dome::create_shader_resources()
{
	make_cube_texture();
	make_text_texture();
	make_sky_dome_texture();
}

void sky_dome::make_cube_texture()
{
	auto device = d3d->get_device();

	auto tex = load_binary_file(L"uv_grid.dds");
	cube_sr = std::make_unique<shader_resource>(device,
	                                            shader_stage::pixel, shader_slot::texture,
	                                            tex);
}

void sky_dome::make_text_texture()
{
	auto device = d3d->get_device();

	auto [width, height] = get_window_size(hWnd);
	auto td = D3D11_TEXTURE2D_DESC{};
	td.Width = width;
	td.Height = height;
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

void sky_dome::make_sky_dome_texture()
{
	auto device = d3d->get_device();

	auto textures = std::vector<std::vector<uint8_t>>{};
	textures.emplace_back(load_binary_file(L"left.dds"));
	textures.emplace_back(load_binary_file(L"right.dds"));
	textures.emplace_back(load_binary_file(L"top.dds"));
	textures.emplace_back(load_binary_file(L"bottom.dds"));
	textures.emplace_back(load_binary_file(L"back.dds"));
	textures.emplace_back(load_binary_file(L"front.dds"));
	
	sky_dome_sr = std::make_unique<shader_resource>(device, 
	                                                shader_stage::pixel, shader_slot::texture,
	                                                textures);
}

void sky_dome::input_update(const game_clock &clk, const raw_input &input)
{
	using btn = input_button;
	using axis = input_axis;

	if (input.is_button_down(input_button::escape))
	{
		stop_drawing = true;
		return;
	}

	auto movement_speed = 5.0f * static_cast<float>(clk.get_delta_s());
	auto dolly = input.which_button_is_down(btn::W, btn::S) * movement_speed;
	auto pan = input.which_button_is_down(btn::D, btn::A) * movement_speed;
	auto crane = input.which_button_is_down(btn::Q, btn::E) * movement_speed;

	fp_cam->translate(dolly, pan, crane);

	auto rotation_speed = 1.0f * static_cast<float>(clk.get_delta_s());
	auto roll = rotation_speed * input.get_axis_value(axis::rx) / 120.0f;
	auto pitch = -rotation_speed * input.get_axis_value(axis::y);
	auto yaw = -rotation_speed * input.get_axis_value(axis::x);

	fp_cam->rotate(roll, pitch, yaw);
}

void sky_dome::cube_update(const game_clock &clk)
{
	auto [width, height] = get_window_size(hWnd);
	auto context = d3d->get_context();

	cube_angle += 90.0f * static_cast<float>(clk.get_delta_s());
	if (cube_angle >= 360.0f)
	{
		cube_angle -= 360.0f;
	}
	
	auto angle = XMConvertToRadians(cube_angle);
	auto cube_pos = matrix{ XMMatrixIdentity() };
	cube_pos.data = XMMatrixRotationY(angle);
	cube_pos.data *= XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	cube_pos.data = XMMatrixTranspose(cube_pos.data);

	cube_cb->update(context, cube_pos);
}

void sky_dome::camera_update()
{
	auto context = d3d->get_context();

	auto view = view_matrix
	{
		XMMatrixTranspose(fp_cam->get_view()),
		fp_cam->get_position()
	};

	view_cb->update(context, view);
}

void sky_dome::text_update(const game_clock &clk)
{
	static long frame_count = 0;
	static auto total_time = 0.0;

	total_time += clk.get_delta_s();
	frame_count++;

	if (total_time < 1.0)
	{
		return;
	}

	auto [width, height] = get_window_size(hWnd);
	auto fps = frame_count / total_time;
	frame_count = 0;
	total_time = 0.0;

	auto fps_text = fmt::format(L"FPS: {:.2f}\nAngle: {:06.2f}", fps, cube_angle);

	auto format = d2d->make_text_format(L"Consolas", 12.0f);
	auto brush = d2d->make_solid_color_brush(D2D1::ColorF(D2D1::ColorF::Yellow));

	d2d->begin_draw(text_sr->get_dxgi_surface(), d2d_clear_color);
	d2d->draw_text(fps_text,
	               { 10, 10 },
	               { static_cast<float>(width), static_cast<float>(height) },
	               format, brush);
	d2d->end();
}

void sky_dome::cube_instance_update(const game_clock &clk)
{
	auto transforms = std::vector<matrix>(100);

	for (auto &&[idx, transform] : transforms | iter::enumerate)
	{
		auto i = static_cast<int>(idx);
		constexpr auto x_max = 10;
		auto x = -15.0f + 3.0f * (i % x_max),
		     z = -30.0f + 3.0f * (i / x_max);
		auto angle = XMConvertToRadians(cube_angle + (i * 10));
		
		transform.data = XMMatrixRotationZ(angle);
		transform.data *= XMMatrixTranslation(x, 0.0f, z);
		transform.data = XMMatrixTranspose(transform.data);
	}

	cube_instance_mb->update_instances(d3d->get_context(), transforms);
}