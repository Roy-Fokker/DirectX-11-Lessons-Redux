#pragma once

#include "pipeline_state.h"

#include <DirectXMath.h>
#include <vector>

namespace dx11_lessons
{
	struct vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};

	static const auto vertex_elements = std::vector<pipeline_state::input_element_type>
	{
		pipeline_state::input_element_type::position,
		pipeline_state::input_element_type::color
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
}