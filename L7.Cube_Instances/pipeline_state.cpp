#include "pipeline_state.h"

#include <DirectXColors.h>
#include <cppitertools/imap.hpp>

using namespace dx11_lessons;

pipeline_state::pipeline_state(device_t device, const description &desc) :
	primitive_topology{ desc.primitive_topology }
{
	create_blend_state(device, desc.blend);
	create_depth_stencil_state(device, desc.depth_stencil);
	create_rasterizer_state(device, desc.rasterizer);
	create_sampler_state(device, desc.sampler);

	create_input_layout(device, desc.input_element_layout, desc.vertex_shader_bytecode);
	create_vertex_shader(device, desc.vertex_shader_bytecode);
	create_pixel_shader(device, desc.pixel_shader_bytecode);
}

pipeline_state::~pipeline_state() = default;

void pipeline_state::activate(context_t context)
{
	context->OMSetBlendState(blend_state,
	                         DirectX::Colors::Transparent,
	                         0xffff'ffff);
	context->OMSetDepthStencilState(depth_stencil_state,
	                                NULL);
	context->RSSetState(rasterizer_state);
	context->PSSetSamplers(0,
	                       1,
	                       &sampler_state.p);


	context->IASetPrimitiveTopology(primitive_topology);
	context->IASetInputLayout(input_layout);

	context->VSSetShader(vertex_shader, nullptr, 0);
	context->PSSetShader(pixel_shader, nullptr, 0);
}

void pipeline_state::create_blend_state(device_t device, blend_type blend)
{
	auto src = D3D11_BLEND{},
	     dst = D3D11_BLEND{};
	auto op = D3D11_BLEND_OP{ D3D11_BLEND_OP_ADD };

	switch (blend)
	{
		case blend_type::opaque:
			src = D3D11_BLEND_ONE;
			dst = D3D11_BLEND_ZERO;
			break;
		case blend_type::alpha:
			src = D3D11_BLEND_ONE;
			dst = D3D11_BLEND_INV_SRC_ALPHA;
			break;
		case blend_type::additive:
			src = D3D11_BLEND_SRC_ALPHA;
			dst = D3D11_BLEND_ONE;
			break;
		case blend_type::non_premultipled:
			src = D3D11_BLEND_SRC_ALPHA;
			dst = D3D11_BLEND_INV_SRC_ALPHA;
			break;
	}

	auto bd = D3D11_BLEND_DESC{};

	bd.RenderTarget[0].BlendEnable = ((src != D3D11_BLEND_ONE) || (dst != D3D11_BLEND_ONE));

	bd.RenderTarget[0].SrcBlend = src;
	bd.RenderTarget[0].BlendOp = op;
	bd.RenderTarget[0].DestBlend = dst;

	bd.RenderTarget[0].SrcBlendAlpha = src;
	bd.RenderTarget[0].BlendOpAlpha = op;
	bd.RenderTarget[0].DestBlendAlpha = dst;

	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	auto hr = device->CreateBlendState(&bd, &blend_state);
	assert(SUCCEEDED(hr));
}

void pipeline_state::create_depth_stencil_state(device_t device, depth_stencil_type depth_stencil)
{
	auto depth_enable{ false }, 
	     write_enable{ false };

	switch (depth_stencil)
	{
		case depth_stencil_type::none:
			depth_enable = false;
			write_enable = false;
			break;
		case depth_stencil_type::read_write:
			depth_enable = true;
			write_enable = true;
			break;
		case depth_stencil_type::read_only:
			depth_enable = true;
			write_enable = false;
			break;
	}

	auto dsd = D3D11_DEPTH_STENCIL_DESC{};

	dsd.DepthEnable = depth_enable;
	dsd.DepthWriteMask = write_enable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	dsd.StencilEnable = false;
	dsd.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsd.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

	dsd.BackFace = dsd.FrontFace;

	auto hr = device->CreateDepthStencilState(&dsd, &depth_stencil_state);
	assert(SUCCEEDED(hr));
}

void pipeline_state::create_rasterizer_state(device_t device, rasterizer_type rasterizer)
{
	auto cull_mode = D3D11_CULL_MODE{};
	auto fill_mode = D3D11_FILL_MODE{};

	switch (rasterizer)
	{
		case rasterizer_type::cull_none:
			cull_mode = D3D11_CULL_NONE;
			fill_mode = D3D11_FILL_SOLID;
			break;
		case rasterizer_type::cull_clockwise:
			cull_mode = D3D11_CULL_FRONT;
			fill_mode = D3D11_FILL_SOLID;
			break;
		case rasterizer_type::cull_anti_clockwise:
			cull_mode = D3D11_CULL_BACK;
			fill_mode = D3D11_FILL_SOLID;
			break;
		case rasterizer_type::wireframe:
			cull_mode = D3D11_CULL_BACK;
			fill_mode = D3D11_FILL_WIREFRAME;
			break;
	}

	auto rd = D3D11_RASTERIZER_DESC{};

	rd.CullMode = cull_mode;
	rd.FillMode = fill_mode;
	rd.DepthClipEnable = true;
	rd.MultisampleEnable = true;

	auto hr = device->CreateRasterizerState(&rd, &rasterizer_state);
	assert(SUCCEEDED(hr));
}

void pipeline_state::create_sampler_state(device_t device, sampler_type sampler)
{
	auto filter = D3D11_FILTER{};
	auto texture_address_mode = D3D11_TEXTURE_ADDRESS_MODE{};

	switch (sampler)
	{
		case sampler_type::point_wrap:
			filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			texture_address_mode = D3D11_TEXTURE_ADDRESS_WRAP;
			break;
		case sampler_type::point_clamp:
			filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			texture_address_mode = D3D11_TEXTURE_ADDRESS_CLAMP;
			break;
		case sampler_type::linear_wrap:
			filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			texture_address_mode = D3D11_TEXTURE_ADDRESS_WRAP;
			break;
		case sampler_type::linear_clamp:
			filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			texture_address_mode = D3D11_TEXTURE_ADDRESS_CLAMP;
			break;
		case sampler_type::anisotropic_wrap:
			filter = D3D11_FILTER_ANISOTROPIC;
			texture_address_mode = D3D11_TEXTURE_ADDRESS_WRAP;
			break;
		case sampler_type::anisotropic_clamp:
			filter = D3D11_FILTER_ANISOTROPIC;
			texture_address_mode = D3D11_TEXTURE_ADDRESS_CLAMP;
			break;
	}

	auto sd = D3D11_SAMPLER_DESC{};

	sd.Filter = filter;

	sd.AddressU = texture_address_mode;
	sd.AddressV = texture_address_mode;
	sd.AddressW = texture_address_mode;

	constexpr auto max_anisotropy = 16u;
	sd.MaxAnisotropy = max_anisotropy;

	sd.MaxLOD = FLT_MAX;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

	auto hr = device->CreateSamplerState(&sd, &sampler_state);
	assert(SUCCEEDED(hr));
}

void pipeline_state::create_input_layout(device_t device, const std::vector<input_element_type> &element_layout, const std::vector<byte> &vso)
{
	constexpr auto position = D3D11_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	constexpr auto normal   = D3D11_INPUT_ELEMENT_DESC{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	constexpr auto color    = D3D11_INPUT_ELEMENT_DESC{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	constexpr auto texcoord = D3D11_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

	auto enum_to_desc = element_layout | iter::imap([&](const input_element_type &iet)
	{
		switch (iet)
		{
			case input_element_type::position:
				return position;
			case input_element_type::normal:
				return normal;
			case input_element_type::color:
				return color;
			case input_element_type::texcoord:
				return texcoord;
		}
		assert(false);
		return D3D11_INPUT_ELEMENT_DESC{};
	});
	auto elements = std::vector(enum_to_desc.begin(), enum_to_desc.end());

	auto hr = device->CreateInputLayout(elements.data(),
	                                    static_cast<uint32_t>(elements.size()),
	                                    vso.data(),
	                                    static_cast<uint32_t>(vso.size()),
	                                    &input_layout);
	assert(SUCCEEDED(hr));
}

void pipeline_state::create_vertex_shader(device_t device, const std::vector<uint8_t> & vso)
{
	auto hr = device->CreateVertexShader(vso.data(),
	                                     vso.size(),
	                                     NULL,
	                                     &vertex_shader);
	assert(SUCCEEDED(hr));
}

void pipeline_state::create_pixel_shader(device_t device, const std::vector<uint8_t> & pso)
{
	auto hr = device->CreatePixelShader(pso.data(),
	                                    pso.size(),
	                                    NULL,
	                                    &pixel_shader);
	assert(SUCCEEDED(hr));
}
