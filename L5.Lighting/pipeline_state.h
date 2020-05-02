#pragma once

#include "direct3d11.h"

#include <atlbase.h>
#include <d3d11_4.h>
#include <vector>
#include <cstddef>

namespace dx11_lessons
{
	class pipeline_state
	{
		using device_t = direct3d11::device_t;
		using context_t = direct3d11::context_t;
		using blend_state_t = CComPtr<ID3D11BlendState>;
		using depth_stencil_state_t = CComPtr<ID3D11DepthStencilState>;
		using rasterizer_state_t = CComPtr<ID3D11RasterizerState>;
		using sampler_state_t = CComPtr<ID3D11SamplerState>;
		using input_layout_t = CComPtr<ID3D11InputLayout>;
		using vertex_shader_t = CComPtr<ID3D11VertexShader>;
		using pixel_shader_t = CComPtr<ID3D11PixelShader>;

	public:
		enum class blend_type
		{
			opaque,
			alpha,
			additive,
			non_premultipled
		};

		enum class depth_stencil_type
		{
			none,
			read_write,
			read_only
		};

		enum class rasterizer_type
		{
			cull_none,
			cull_clockwise,
			cull_anti_clockwise,
			wireframe
		};

		enum class sampler_type
		{
			point_wrap,
			point_clamp,
			linear_wrap,
			linear_clamp,
			anisotropic_wrap,
			anisotropic_clamp
		};

		enum class input_element_type
		{
			position,
			normal,
			color,
			texcoord,
		};

		struct description
		{
			blend_type blend;
			depth_stencil_type depth_stencil;
			rasterizer_type rasterizer;
			sampler_type sampler;

			const std::vector<input_element_type> &input_element_layout;
			const std::vector<uint8_t> &vertex_shader_bytecode;
			const std::vector<uint8_t> &pixel_shader_bytecode;

			D3D11_PRIMITIVE_TOPOLOGY primitive_topology;
		};

	public:
		pipeline_state() = delete;
		pipeline_state(device_t device, const description &desc);
		~pipeline_state();

		void activate(context_t context);

	private:
		void create_blend_state(device_t device, blend_type blend);
		void create_depth_stencil_state(device_t device, depth_stencil_type depth_stencil);
		void create_rasterizer_state(device_t device, rasterizer_type rasterizer);
		void create_sampler_state(device_t device, sampler_type sampler);

		void create_input_layout(device_t device, 
		                         const std::vector<input_element_type> &input_layout,
		                         const std::vector<byte> &vso);
		void create_vertex_shader(device_t device, const std::vector<uint8_t> &vso);
		void create_pixel_shader(device_t device, const std::vector<uint8_t> &pso);

	private:
		blend_state_t blend_state{};
		depth_stencil_state_t depth_stencil_state{};
		rasterizer_state_t rasterizer_state{};
		sampler_state_t sampler_state{};

		input_layout_t input_layout{};
		vertex_shader_t vertex_shader{};
		pixel_shader_t pixel_shader{};

		D3D11_PRIMITIVE_TOPOLOGY primitive_topology{};
	};
}