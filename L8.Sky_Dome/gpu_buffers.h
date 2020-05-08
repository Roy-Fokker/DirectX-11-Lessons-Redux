#pragma once

#include "direct3d11.h"
#include "gpu_datatypes.h"

#include <functional>
#include <vector>
#include <cstdint>

namespace dx11_lessons
{
	enum class shader_stage
	{
		vertex,
		pixel,
	};

	enum class shader_slot
	{
		projection = 0,
		view = 1,
		transform = 2,
		texture = 0,
		light = 0,
	};

	class mesh_buffer
	{
	public:
		mesh_buffer() = delete;
		mesh_buffer(direct3d11::device_t device, const mesh &data);
		mesh_buffer(direct3d11::device_t device, const instanced_mesh &data);
		~mesh_buffer();

		void update_instances(direct3d11::context_t context, const std::vector<matrix> &data);
		void activate(direct3d11::context_t context);
		void draw(direct3d11::context_t context);

	private:
		using buffer_t = CComPtr<ID3D11Buffer>;
		std::vector<buffer_t> vertex_buffers{};
		std::vector<uint32_t> buffer_strides{},
		                      buffer_offsets{};

		buffer_t index_buffer;
		uint32_t index_count{},
		         index_offset{},
		         instance_count{};
	};

	class constant_buffer
	{
	public:
		template <class T>
		constant_buffer(direct3d11::device_t device, shader_stage stage, shader_slot slot, const T &data_object) :
			constant_buffer(device, stage, slot, sizeof(T), reinterpret_cast<const void *>(&data_object))
		{};

		template <class T>
		void update(direct3d11::context_t context, const T &data_object)
		{
			update(context, sizeof(T), reinterpret_cast<const void *>(&data_object));
		};

	public:
		constant_buffer() = delete;
		constant_buffer(direct3d11::device_t device, shader_stage stage, shader_slot slot, 
		                std::size_t buffer_size, const void *buffer_data);
		~constant_buffer();

		void activate(direct3d11::context_t context);
		void update(direct3d11::context_t context, std::size_t buffer_size, const void *buffer_data);

	private:
		void create_set_function();

	private:
		const std::size_t buffer_size;
		CComPtr<ID3D11Buffer> buffer;
		shader_stage stage;
		shader_slot slot;

		using set_buffer_fn = void (*)(direct3d11::context_t, 
		                               uint32_t slot, uint32_t count, 
		                               ID3D11Buffer *const *);

		set_buffer_fn set_buffer;
	};

	class shader_resource
	{
	public:
		using texture_t = CComPtr<ID3D11Texture2D>;

		shader_resource() = delete;
		shader_resource(direct3d11::device_t device, shader_stage stage, shader_slot slot, 
		                const std::vector<uint8_t> &data);
		shader_resource(direct3d11::device_t device, shader_stage stage, shader_slot slot,
		                const texture_t texture);
		shader_resource(direct3d11::device_t device, shader_stage stage, shader_slot slot,
		                const std::vector<std::vector<uint8_t>> &data);
		~shader_resource();

		void activate(direct3d11::context_t context);

		auto get_dxgi_surface() const -> CComPtr<IDXGISurface1>;

	private:
		void create_set_function();

	private:
		using resource_t = CComPtr<ID3D11Resource>;
		resource_t resource;
		CComPtr<ID3D11ShaderResourceView> resource_view;
		shader_stage stage;
		shader_slot slot;

		using set_resource_fn = void (*)(direct3d11::context_t,
		                                 uint32_t slot, uint32_t count,
		                                 ID3D11ShaderResourceView *const *);

		set_resource_fn set_resource;
	};
}