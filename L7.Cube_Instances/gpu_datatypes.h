#pragma once

#include "pipeline_state.h"

#include <DirectXMath.h>
#include <vector>

namespace dx11_lessons
{
	struct vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texcoord;
	};

	static const auto vertex_elements = std::vector<pipeline_state::input_element_type>
	{
		pipeline_state::input_element_type::position,
		pipeline_state::input_element_type::normal,
		pipeline_state::input_element_type::texcoord
	};

	struct mesh
	{
		std::vector<vertex> vertices;
		std::vector<uint32_t> indicies;
	};

	struct matrix
	{
		DirectX::XMMATRIX data;
	};

	struct instanced_mesh
	{
		std::vector<vertex> vertices;
		std::vector<uint32_t> indicies;
		std::vector<matrix> instance_transforms;
	};

	static const auto instanced_vertex_elements = std::vector<pipeline_state::input_element_type>
	{
		pipeline_state::input_element_type::position,
		pipeline_state::input_element_type::normal,
		pipeline_state::input_element_type::texcoord,

		pipeline_state::input_element_type::instance_float4,
		pipeline_state::input_element_type::instance_float4,
		pipeline_state::input_element_type::instance_float4,
		pipeline_state::input_element_type::instance_float4,
	};

	struct view_matrix
	{
		DirectX::XMMATRIX matrix;
		DirectX::XMVECTOR position;
	};

	struct light
	{
		DirectX::XMFLOAT4 diffuse;
		DirectX::XMFLOAT4 ambient;
		DirectX::XMFLOAT3 light_pos;
		float specular_power;
		DirectX::XMFLOAT4 specular;
	};
}