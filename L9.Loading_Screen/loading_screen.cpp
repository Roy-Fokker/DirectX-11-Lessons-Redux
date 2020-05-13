#include "loading_screen.h"

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

		result_mesh.vertices.shrink_to_fit();
		result_mesh.indicies.shrink_to_fit();

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

	enum ps_ids
	{
		ps_default,
		ps_text,
		ps_light,
		ps_instancing,
		ps_sky,
	};

	enum mb_ids
	{
		mb_cube,
		mb_cubes,
		mb_text,
		mb_sky,
	};

	enum cb_ids
	{
		cb_prespective,
		cb_orthographic,
		cb_camera,
		cb_text,
		cb_cube,
		cb_light,
	};

	enum sr_ids
	{
		sr_text,
		sr_cube,
		sr_sky,
	};
}

loading_screen::loading_screen(HWND hwnd) :
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

loading_screen::~loading_screen() = default;

auto loading_screen::on_resize(uintptr_t wParam, uintptr_t lParam) -> bool
{
	rp.reset(nullptr);

	d3d->resize();

	rp = std::make_unique<render_pass>(d3d->get_device(), d3d->get_swapchain());
	make_prespective_cb();
	make_orthographic_cb();
	make_text_texture();

	return true;
}

auto loading_screen::exit() const -> bool
{
	return stop_drawing;
}

void loading_screen::update(const game_clock &clk, const raw_input &input)
{
	input_update(clk, input);
	cube_update(clk);
	camera_update();
	cube_instance_update(clk);
	text_update(clk);
}

void loading_screen::render()
{
	auto context = d3d->get_context();

	rp->activate(context);
	rp->clear(context, clear_color);

	constant_buffers[cb_prespective]->activate(context);
	constant_buffers[cb_camera]->activate(context);
	draw_cube();
	draw_cube_instances();
	draw_sky();

	constant_buffers[cb_orthographic]->activate(context);
	draw_text();

	d3d->present(enable_vSync);
}

void loading_screen::create_pipeline_state_object()
{
	pipeline_states.resize(5);

	make_default_ps();
	make_light_ps();
	make_text_ps();
	make_cube_instance_ps();
	make_sky_dome_ps();
}

void loading_screen::make_default_ps()
{
	auto device = d3d->get_device();
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
	pipeline_states[ps_default] = std::make_unique<pipeline_state>(device, desc);
}

void loading_screen::make_light_ps()
{
	auto device = d3d->get_device();
	auto vso = load_binary_file(L"vertex_shader.cso"),
	     pso = load_binary_file(L"lighting.ps.cso");

	auto light_desc = pipeline_state::description
	{
		bs::alpha,
		ds::read_write,
		rs::cull_anti_clockwise,
		ss::anisotropic_clamp,
		vertex_elements,
		vso,
		pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};

	pipeline_states[ps_light] = std::make_unique<pipeline_state>(device, light_desc);
}

void loading_screen::make_text_ps()
{
	auto device = d3d->get_device();
	auto vso = load_binary_file(L"screen_space_text.vs.cso"),
	     pso = load_binary_file(L"pixel_shader.cso");

	auto screen_text_desc = pipeline_state::description
	{
		bs::non_premultipled,
		ds::read_write,
		rs::cull_anti_clockwise,
		ss::anisotropic_clamp,
		vertex_elements,
		vso,
		pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	pipeline_states[ps_text]= std::make_unique<pipeline_state>(device, screen_text_desc);
}

void loading_screen::make_cube_instance_ps()
{
	auto device = d3d->get_device();
	auto vso = load_binary_file(L"cube_instances.vs.cso"),
	     pso = load_binary_file(L"pixel_shader.cso");

	auto desc = pipeline_state::description
	{
		bs::opaque,
		ds::read_write,
		rs::cull_anti_clockwise,
		ss::anisotropic_clamp,
		instanced_vertex_elements,
		vso,
		pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};
	pipeline_states[ps_instancing] = std::make_unique<pipeline_state>(device, desc);
}

void loading_screen::make_sky_dome_ps()
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
		vso,
		pso,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	};

	pipeline_states[ps_sky] = std::make_unique<pipeline_state>(device, sky_dome_desc);
}

void loading_screen::create_mesh_buffers()
{
	mesh_buffers.resize(4);

	make_cube_mesh();
	make_text_mesh();
	make_cube_instance_mesh();
	make_sky_dome_mesh();
}

void loading_screen::make_cube_mesh()
{
	auto device = d3d->get_device();
	auto cube_mesh = spherify_and_invert(cube_base, 2);
	mesh_buffers[mb_cube] = std::make_unique<mesh_buffer>(device, cube_mesh);
}

void loading_screen::make_cube_instance_mesh()
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

	mesh_buffers[mb_cubes] = std::make_unique<mesh_buffer>(device, cube_mesh);
}

void loading_screen::make_text_mesh()
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

	mesh_buffers[mb_text] = std::make_unique<mesh_buffer>(device, rect_mesh);
}

void loading_screen::make_sky_dome_mesh()
{
	auto device = d3d->get_device();
	auto dome = spherify_and_invert(cube_base, 2);
	mesh_buffers[mb_sky] = std::make_unique<mesh_buffer>(device, dome);
}

void loading_screen::create_contant_buffers()
{
	constant_buffers.resize(6);

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

void loading_screen::make_prespective_cb()
{
	auto device = d3d->get_device();
	auto [width, height] = get_window_size(hWnd);

	auto aspect_ratio = width / static_cast<float>(height);
	auto h_fov = XMConvertToRadians(field_of_view);
	auto v_fov = 2.0f * std::atan(std::tan(h_fov / 2.0f) * aspect_ratio);

	auto projection = matrix{};
	projection.data = XMMatrixPerspectiveFovLH(v_fov, aspect_ratio, near_z, far_z);
	projection.data = XMMatrixTranspose(projection.data);
	constant_buffers[cb_prespective] = std::make_unique<constant_buffer>(device, stage::vertex, slot::projection, projection);
}

void loading_screen::make_orthographic_cb()
{
	auto device = d3d->get_device();
	auto [width, height] = get_window_size(hWnd);

	auto projection = matrix{};
	projection.data = XMMatrixOrthographicLH(width, height, -1.0f, 1.0f);
	projection.data = XMMatrixTranspose(projection.data);
	constant_buffers[cb_orthographic] = std::make_unique<constant_buffer>(device, stage::vertex, slot::projection, projection);
}

void loading_screen::make_view_cb()
{
	auto device = d3d->get_device();

	fp_cam = std::make_unique<camera>(XMFLOAT3{ 0.0f, 2.0f, 5.0f },
	                                  XMFLOAT3{ 0.0f, 0.0f, 0.0f },
	                                  XMFLOAT3{ 0.0f, 1.0f, 0.0f });
	auto view = view_matrix{};
	view.matrix = XMMatrixTranspose(fp_cam->get_view());
	view.position = fp_cam->get_position();
	constant_buffers[cb_camera] = std::make_unique<constant_buffer>(device, stage::vertex, slot::view, view);
}

void loading_screen::make_cube_transform_cb()
{
	auto device = d3d->get_device();

	auto cube_pos = matrix{ XMMatrixTranslation(0.0f, 1.0f, 0.0f) };
	cube_pos.data = XMMatrixTranspose(cube_pos.data);
	constant_buffers[cb_cube] = std::make_unique<constant_buffer>(device, stage::vertex, slot::transform, cube_pos);

}

void loading_screen::make_text_transform_cb()
{
	auto device = d3d->get_device();

	auto text_pos = matrix{ XMMatrixTranslation(0.0f, 0.0f, 0.0f) };
	text_pos.data = XMMatrixTranspose(text_pos.data);
	constant_buffers[cb_text] = std::make_unique<constant_buffer>(device, stage::vertex, slot::transform, text_pos);
}

void loading_screen::make_light_data_cb()
{
	auto device = d3d->get_device();

	auto light_data = light{};
	light_data.diffuse = { 0.0f, 0.0f, 1.0f, 1.0f };
	light_data.ambient = { 0.0f, 1.0f, 0.0f, 1.0f };
	light_data.light_pos = { 0.0f, 3.0f, 5.0f };
	light_data.specular_power = 32.0f;
	light_data.specular = { 1.0f, 0.0f, 0.0f, 1.0f };
	constant_buffers[cb_light] = std::make_unique<constant_buffer>(device, stage::pixel, slot::light, light_data);
}

void loading_screen::create_shader_resources()
{
	shader_resources.resize(3);

	make_cube_texture();
	make_text_texture();
	make_sky_dome_texture();
}

void loading_screen::make_cube_texture()
{
	auto device = d3d->get_device();

	auto tex = load_binary_file(L"uv_grid.dds");
	shader_resources[sr_cube] = std::make_unique<shader_resource>(device,
	                                            shader_stage::pixel, shader_slot::texture,
	                                            tex);
}

void loading_screen::make_text_texture()
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

	shader_resources[sr_text] = std::make_unique<shader_resource>(device,
	                                            shader_stage::pixel, shader_slot::texture,
	                                            text_tex);
}

void loading_screen::make_sky_dome_texture()
{
	auto device = d3d->get_device();

	auto textures = std::vector<std::vector<uint8_t>>{};
	textures.emplace_back(load_binary_file(L"left.dds"));
	textures.emplace_back(load_binary_file(L"right.dds"));
	textures.emplace_back(load_binary_file(L"top.dds"));
	textures.emplace_back(load_binary_file(L"bottom.dds"));
	textures.emplace_back(load_binary_file(L"back.dds"));
	textures.emplace_back(load_binary_file(L"front.dds"));
	
	shader_resources[sr_sky] = std::make_unique<shader_resource>(device,
	                                                shader_stage::pixel, shader_slot::texture,
	                                                textures);
}

void loading_screen::input_update(const game_clock &clk, const raw_input &input)
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

void loading_screen::cube_update(const game_clock &clk)
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

	constant_buffers[cb_cube]->update(context, cube_pos);
}

void loading_screen::camera_update()
{
	auto context = d3d->get_context();

	auto view = view_matrix
	{
		XMMatrixTranspose(fp_cam->get_view()),
		fp_cam->get_position()
	};

	constant_buffers[cb_camera]->update(context, view);
}

void loading_screen::text_update(const game_clock &clk)
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

	d2d->begin_draw(shader_resources[sr_text]->get_dxgi_surface(), d2d_clear_color);
	d2d->draw_text(fps_text,
	               { 10, 10 },
	               { static_cast<float>(width), static_cast<float>(height) },
	               format, brush);
	d2d->end();
}

void loading_screen::cube_instance_update(const game_clock &clk)
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

	mesh_buffers[mb_cubes]->update_instances(d3d->get_context(), transforms);
}

void loading_screen::draw_cube()
{
	auto context = d3d->get_context();
	
	pipeline_states[ps_light]->activate(context);
	constant_buffers[cb_light]->activate(context);
	constant_buffers[cb_cube]->activate(context);
	shader_resources[sr_cube]->activate(context);
	mesh_buffers[mb_cube]->activate(context);

	mesh_buffers[mb_cube]->draw(context);
}

void loading_screen::draw_cube_instances()
{
	auto context = d3d->get_context();

	pipeline_states[ps_instancing]->activate(context);
	shader_resources[sr_cube]->activate(context);
	mesh_buffers[mb_cubes]->activate(context);

	mesh_buffers[mb_cubes]->draw(context);
}

void loading_screen::draw_text()
{
	auto context = d3d->get_context();

	pipeline_states[ps_text]->activate(context);
	shader_resources[sr_text]->activate(context);
	constant_buffers[cb_text]->activate(context);
	mesh_buffers[mb_text]->activate(context);

	mesh_buffers[mb_text]->draw(context);
}

void loading_screen::draw_sky()
{
	auto context = d3d->get_context();

	pipeline_states[ps_sky]->activate(context);
	shader_resources[sr_sky]->activate(context);
	mesh_buffers[mb_sky]->activate(context);

	mesh_buffers[mb_sky]->draw(context);
}
