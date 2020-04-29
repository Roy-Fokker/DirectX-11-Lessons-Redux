#pragma once

#include "direct3d11.h"
#include "gpu_datatypes.h"

namespace dx11_lessons
{
	class mesh_buffer
	{
	public:
		mesh_buffer() = delete;
		mesh_buffer(direct3d11::device_t device, const mesh &data);
		~mesh_buffer();

		void activate(direct3d11::context_t context);
		void draw(direct3d11::context_t context);

	private:
		CComPtr<ID3D11Buffer> vertex_buffer,
		                      index_buffer;

		uint32_t index_count{},
		         index_offset{},
		         vertex_size{},
		         vertex_offset{};
	};

	class constant_buffer
	{
	public:
		enum class shader_stage
		{
			vertex, 
			pixel,
		};

		enum class shader_slot
		{
			projection, 
			view, 
			transform,
		};

	public:
		constant_buffer() = delete;
		constant_buffer(direct3d11::device_t device, shader_stage stage, shader_slot slot, 
		                std::size_t buffer_size, const void *buffer_data);
		~constant_buffer();

		void activate(direct3d11::context_t context);
		void update(direct3d11::context_t context, std::size_t buffer_size, const void *buffer_data);

	private:
		const std::size_t buffer_size;
		CComPtr<ID3D11Buffer> buffer;
		shader_stage stage;
		shader_slot slot;
	};
}