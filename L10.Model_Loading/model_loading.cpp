#include "model_loading.h"

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
#include <thread>
#include <future>
#include <string_view>
#include <functional>

using namespace dx11_lessons;
using namespace DirectX;
using namespace std::string_view_literals;

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
		ps_sky,
	};

	enum mb_ids
	{
		mb_text,
		mb_sky,
	};

	enum cb_ids
	{
		cb_prespective,
		cb_orthographic,
		cb_camera,
		cb_text,
	};

	enum sr_ids
	{
		sr_text,
		sr_sky,
	};

	auto list_of_files_to_load = std::array{
		L"vertex_shader.cso"sv,
		L"pixel_shader.cso"sv,
		L"screen_space_text.vs.cso"sv,
		L"sky_dome.vs.cso"sv,
		L"sky_dome.ps.cso"sv,
		L"left.dds"sv,
		L"right.dds"sv,
		L"top.dds"sv,
		L"bottom.dds"sv,
		L"back.dds"sv,
		L"front.dds"sv,
	};

	enum file_list
	{
		basic_vso, 
		basic_pso,
		text_vso,
		sky_vso,
		sky_pso,
		left_tex,
		right_tex,
		top_tex,
		bottom_tex,
		back_tex,
		front_tex,
	};
}

model_loading::model_loading(HWND hwnd) :
	hWnd{ hwnd }
{
	d3d = std::make_unique<direct3d11>(hWnd);
	d2d = std::make_unique<direct2d1>(d3d->get_dxgi_device());
	rp = std::make_unique<render_pass>(d3d->get_device(), d3d->get_swapchain());

	load_files();
	create_pipeline_state_object();
	create_mesh_buffers();
	create_contant_buffers();
	create_shader_resources();
}

model_loading::~model_loading() = default;

auto model_loading::on_resize(uintptr_t wParam, uintptr_t lParam) -> bool
{
	rp.reset(nullptr);

	d3d->resize();

	rp = std::make_unique<render_pass>(d3d->get_device(), d3d->get_swapchain());
	make_prespective_cb();
	make_orthographic_cb();
	make_text_texture();

	return true;
}

auto model_loading::exit() const -> bool
{
	return stop_drawing;
}

void model_loading::update(const game_clock &clk, const raw_input &input)
{
	auto all_good = true;
	for (auto &f : object_futures)
	{
		all_good = all_good 
		       and (f._Is_ready());
	}
	if (all_good
		and
		not object_futures.empty())
	{
		object_futures.clear();
		files_loaded.clear();
	}

	if (all_good)
	{
		input_update(clk, input);
		camera_update();
		text_update(clk);
	}
	else
	{
		update_load_status();
	}
}

void model_loading::render()
{
	auto context = d3d->get_context();
	rp->activate(context);
	rp->clear(context, clear_color);

	if (not object_futures.empty())
	{
		draw_load_status();
	}
	else
	{
		constant_buffers[cb_prespective]->activate(context);
		constant_buffers[cb_camera]->activate(context);
		
		draw_sky();

		constant_buffers[cb_orthographic]->activate(context);
		draw_text();
	}

	d3d->present(enable_vSync);
}

void model_loading::load_files()
{
	files_loaded.resize(list_of_files_to_load.size());

	auto load_file_async = [&](uint16_t idx, std::wstring_view file_name)
	{
		files_loaded[idx] = load_binary_file(file_name);
		return true;
	};

	for (auto &&[i, filename] : list_of_files_to_load | iter::enumerate)
	{
		object_futures.emplace_back(
			std::async(std::launch::async, load_file_async, static_cast<uint16_t>(i), filename));
	}
}

void model_loading::create_pipeline_state_object()
{
	pipeline_states.resize(5);

	make_text_ps();

	object_futures.emplace_back(
		std::async(std::launch::async, [&]
	{
		make_default_ps();
		return true;
	}));

	object_futures.emplace_back(
		std::async(std::launch::async, [&]
	{
		make_sky_dome_ps();
		return true;
	}));
}

void model_loading::make_default_ps()
{
	if (not object_futures[basic_vso]._Is_ready())
	{
		object_futures[basic_vso].wait();
	}
	if (not object_futures[basic_pso]._Is_ready())
	{
		object_futures[basic_pso].wait();
	}

	auto device = d3d->get_device();
	auto &vso = files_loaded[basic_vso], //load_binary_file(L"vertex_shader.cso"),
	     &pso = files_loaded[basic_pso]; //load_binary_file(L"pixel_shader.cso");

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

void model_loading::make_text_ps()
{
	if (not object_futures[text_vso]._Is_ready())
	{
		object_futures[text_vso].wait();
	}
	if (not object_futures[basic_pso]._Is_ready())
	{
		object_futures[basic_pso].wait();
	}

	auto device = d3d->get_device();
	auto &vso = files_loaded[text_vso], // load_binary_file(L"screen_space_text.vs.cso"),
		&pso = files_loaded[basic_pso]; // load_binary_file(L"pixel_shader.cso");

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

void model_loading::make_sky_dome_ps()
{
	if (not object_futures[sky_vso]._Is_ready())
	{
		object_futures[sky_vso].wait();
	}
	if (not object_futures[sky_pso]._Is_ready())
	{
		object_futures[sky_pso].wait();
	}

	auto device = d3d->get_device();
	auto &vso = files_loaded[sky_vso], // load_binary_file(L"sky_dome.vs.cso"),
		&pso = files_loaded[sky_pso]; // load_binary_file(L"sky_dome.ps.cso");

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

void model_loading::create_mesh_buffers()
{
	mesh_buffers.resize(4);

	make_text_mesh();

	object_futures.emplace_back(
		std::async(std::launch::async, [&]
	{
		make_sky_dome_mesh();
		return true;
	}));
}

void model_loading::make_text_mesh()
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

void model_loading::make_sky_dome_mesh()
{
	auto device = d3d->get_device();
	auto dome = spherify_and_invert(cube_base, 2);
	mesh_buffers[mb_sky] = std::make_unique<mesh_buffer>(device, dome);
}

void model_loading::create_contant_buffers()
{
	constant_buffers.resize(6);

	make_orthographic_cb();
	make_text_transform_cb();

	object_futures.emplace_back(
		std::async(std::launch::async, [&]
	{
		make_prespective_cb();
		return true;
	}));
	object_futures.emplace_back(
		std::async(std::launch::async, [&]
	{
		make_view_cb();
		return true;
	}));
}

void model_loading::make_prespective_cb()
{
	auto device = d3d->get_device();
	auto [width, height] = get_window_size(hWnd);

	constexpr auto h_fov = XMConvertToRadians(field_of_view);
	auto aspect_ratio = width / static_cast<float>(height);
	auto v_fov = 2.0f * std::atan(std::tan(h_fov / 2.0f) * aspect_ratio);

	auto projection = matrix{};
	projection.data = XMMatrixPerspectiveFovLH(v_fov, aspect_ratio, near_z, far_z);
	projection.data = XMMatrixTranspose(projection.data);
	constant_buffers[cb_prespective] = std::make_unique<constant_buffer>(device, stage::vertex, slot::projection, projection);
}

void model_loading::make_orthographic_cb()
{
	auto device = d3d->get_device();
	auto [width, height] = get_window_size(hWnd);

	auto projection = matrix{};
	projection.data = XMMatrixOrthographicLH(width, height, -1.0f, 1.0f);
	projection.data = XMMatrixTranspose(projection.data);
	constant_buffers[cb_orthographic] = std::make_unique<constant_buffer>(device, stage::vertex, slot::projection, projection);
}

void model_loading::make_view_cb()
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

void model_loading::make_text_transform_cb()
{
	auto device = d3d->get_device();

	auto text_pos = matrix{ XMMatrixTranslation(0.0f, 0.0f, 0.0f) };
	text_pos.data = XMMatrixTranspose(text_pos.data);
	constant_buffers[cb_text] = std::make_unique<constant_buffer>(device, stage::vertex, slot::transform, text_pos);
}

void model_loading::create_shader_resources()
{
	shader_resources.resize(3);

	make_text_texture();

	object_futures.emplace_back(
		std::async(std::launch::async, [&]
	{
		make_sky_dome_texture();
		return true;
	}));
}

void model_loading::make_text_texture()
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

void model_loading::make_sky_dome_texture()
{
	auto textures = std::vector<std::vector<uint8_t>>{};

	auto copy_loaded_file = [&](uint16_t file_idx)
	{
		if (not object_futures[file_idx]._Is_ready())
		{
			object_futures[file_idx].wait();
		}

		textures.push_back(files_loaded[file_idx]);
	};

	
	copy_loaded_file(left_tex);
	copy_loaded_file(right_tex);
	copy_loaded_file(top_tex);
	copy_loaded_file(bottom_tex);
	copy_loaded_file(back_tex);
	copy_loaded_file(front_tex);

	auto device = d3d->get_device();

	shader_resources[sr_sky] = std::make_unique<shader_resource>(device,
																 shader_stage::pixel, shader_slot::texture,
																 textures);
}

void model_loading::input_update(const game_clock &clk, const raw_input &input)
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

	auto rotation_speed = XMConvertToRadians(field_of_view) * static_cast<float>(clk.get_delta_s());
	auto roll = rotation_speed * input.get_axis_value(axis::rx) / 120.0f;
	auto pitch = -rotation_speed * input.get_axis_value(axis::y);
	auto yaw = -rotation_speed * input.get_axis_value(axis::x);

	fp_cam->rotate(roll, pitch, yaw);
}

void model_loading::camera_update()
{
	auto context = d3d->get_context();

	auto view = view_matrix
	{
		XMMatrixTranspose(fp_cam->get_view()),
		fp_cam->get_position()
	};

	constant_buffers[cb_camera]->update(context, view);
}

void model_loading::text_update(const game_clock &clk)
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

	auto fps_text = fmt::format(L"FPS: {:.2f}\n", fps);

	auto format = d2d->make_text_format(L"Consolas", 12.0f);
	auto brush = d2d->make_solid_color_brush(D2D1::ColorF(D2D1::ColorF::Yellow));

	d2d->begin_draw(shader_resources[sr_text]->get_dxgi_surface(), d2d_clear_color);
	d2d->draw_text(fps_text,
	               { 10, 10 },
	               { static_cast<float>(width), static_cast<float>(height) },
	               format, brush);
	d2d->end();
}

void model_loading::draw_text()
{
	auto context = d3d->get_context();

	pipeline_states[ps_text]->activate(context);
	shader_resources[sr_text]->activate(context);
	constant_buffers[cb_text]->activate(context);
	mesh_buffers[mb_text]->activate(context);

	mesh_buffers[mb_text]->draw(context);
}

void model_loading::draw_sky()
{
	auto context = d3d->get_context();

	pipeline_states[ps_sky]->activate(context);
	shader_resources[sr_sky]->activate(context);
	mesh_buffers[mb_sky]->activate(context);

	mesh_buffers[mb_sky]->draw(context);
}

void model_loading::update_load_status()
{
	auto loaded_items = std::count_if(pipeline_states.begin(), pipeline_states.end(),
								  [](const std::unique_ptr<pipeline_state> &ptr)
	{
		return ptr != nullptr;
	});
	loaded_items += std::count_if(constant_buffers.begin(), constant_buffers.end(),
								  [](const std::unique_ptr<constant_buffer> &ptr)
	{
		return ptr != nullptr;
	});
	loaded_items += std::count_if(mesh_buffers.begin(), mesh_buffers.end(),
								  [](const std::unique_ptr<mesh_buffer> &ptr)
	{
		return ptr != nullptr;
	});
	loaded_items += std::count_if(shader_resources.begin(), shader_resources.end(),
								  [](const std::unique_ptr<shader_resource> &ptr)
	{
		return ptr != nullptr;
	});
	loaded_items += std::count_if(files_loaded.begin(), files_loaded.end(),
								  [](const std::vector<uint8_t> &ptr)
	{
		return not ptr.empty();
	});
	loaded_items -= 5;

	auto text = fmt::format(L"Loaded: {} of {}", loaded_items, object_futures.size());
	auto [width, height] = get_window_size(hWnd);
	auto format = d2d->make_text_format(L"Consolas", 20.0f);
	auto brush = d2d->make_solid_color_brush(D2D1::ColorF(D2D1::ColorF::Yellow));

	d2d->begin_draw(shader_resources[sr_text]->get_dxgi_surface(), d2d_clear_color);
	d2d->draw_text(text,
				   { 10, 10 },
				   { static_cast<float>(width), static_cast<float>(height) },
				   format, brush);
	d2d->end();
}

void model_loading::draw_load_status()
{
	auto context = d3d->get_context();
	constant_buffers[cb_orthographic]->activate(context);
	draw_text();
}
