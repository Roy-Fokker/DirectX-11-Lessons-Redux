#include "gpu_buffers.h"

#include <DirectXTK\DDSTextureLoader.h>
#include <array>
#include <cassert>

using namespace dx11_lessons;

namespace 
{
	using buffer_t = CComPtr<ID3D11Buffer>;
	using device_t = direct3d11::device_t;
	using context_t = direct3d11::context_t;

	auto make_gpu_buffer(device_t device, const D3D11_BUFFER_DESC &desc, const D3D11_SUBRESOURCE_DATA &data) -> buffer_t
	{
		buffer_t buffer{};
		auto hr = device->CreateBuffer(&desc, &data, &buffer);
		assert(SUCCEEDED(hr));

		return buffer;
	}
}

#pragma region Mesh Buffer
mesh_buffer::mesh_buffer(device_t device, const mesh &data) :
	index_count{ static_cast<uint32_t>(data.indicies.size()) },
	buffer_strides{ sizeof(data.vertices.back()) }, 
	buffer_offsets{ 0 }
{
	auto &vertices = data.vertices;
	auto &indicies = data.indicies;

	auto desc = D3D11_BUFFER_DESC{};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = NULL;
	
	auto srd = D3D11_SUBRESOURCE_DATA{};

	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = static_cast<uint32_t>(vertices.size()) * buffer_strides.at(0);
	srd.pSysMem = reinterpret_cast<const void *>(vertices.data());
	vertex_buffers.push_back( make_gpu_buffer(device, desc, srd) );

	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.ByteWidth = index_count * sizeof(uint32_t);
	srd.pSysMem = reinterpret_cast<const void *>(indicies.data());
	index_buffer = make_gpu_buffer(device, desc, srd);
}

mesh_buffer::mesh_buffer(direct3d11::device_t device, const instanced_mesh &data) :
	mesh_buffer(device, mesh{ data.vertices, data.indicies })
{
	buffer_strides.push_back(sizeof(data.instance_transforms.back()));
	buffer_offsets.push_back(0);
	instance_count = static_cast<uint32_t>(data.instance_transforms.size());

	auto &transforms = data.instance_transforms;

	auto desc = D3D11_BUFFER_DESC{};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = instance_count * buffer_strides.at(1);

	auto srd = D3D11_SUBRESOURCE_DATA{};
	srd.pSysMem = reinterpret_cast<const void *>(transforms.data());
	vertex_buffers.push_back(make_gpu_buffer(device, desc, srd));
}

mesh_buffer::~mesh_buffer() = default;

void mesh_buffer::update_instances(direct3d11::context_t context, const std::vector<matrix> &buffer_data)
{
	assert(instance_count >= buffer_data.size());

	auto &buffer = vertex_buffers.at(1);
	auto buffer_size = buffer_data.size() * buffer_strides.at(1);

	auto gpu_buffer = D3D11_MAPPED_SUBRESOURCE{};
	auto hr = context->Map(buffer.p,
	                       NULL,
	                       D3D11_MAP_WRITE_DISCARD,
	                       NULL,
	                       &gpu_buffer);
	assert(SUCCEEDED(hr));

	std::memcpy(gpu_buffer.pData, &buffer_data[0], buffer_size);

	context->Unmap(buffer.p, NULL);
}

void mesh_buffer::activate(context_t context)
{
	auto buffers = std::vector<ID3D11Buffer *const *>{};
	for (auto &vb : vertex_buffers)
	{
		buffers.push_back(&vb.p);
	}
	context->IASetVertexBuffers(0, 
	                            static_cast<uint32_t>(buffers.size()), 
	                            buffers[0], 
	                            buffer_strides.data(), 
	                            buffer_offsets.data());
	context->IASetIndexBuffer(index_buffer.p, DXGI_FORMAT_R32_UINT, index_offset);
}

void mesh_buffer::draw(context_t context)
{
	if (vertex_buffers.size() == 1)
	{
		context->DrawIndexed(index_count, 0, 0);
	}
	else
	{
		context->DrawIndexedInstanced(index_count, instance_count, 0, 0, 0);
	}
}
#pragma endregion

#pragma region Constant Buffer
constant_buffer::constant_buffer(device_t device, shader_stage stage_, shader_slot slot_, std::size_t size, const void *data) :
	stage{ stage_ },
	slot{ slot_ },
	buffer_size{ size }
{
	auto desc = D3D11_BUFFER_DESC{};
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.ByteWidth = static_cast<uint32_t>(buffer_size);

	auto srd = D3D11_SUBRESOURCE_DATA{};
	srd.pSysMem = data;

	buffer = make_gpu_buffer(device, desc, srd);

	create_set_function();
}

constant_buffer::~constant_buffer() = default;

void constant_buffer::activate(context_t context)
{
	ID3D11Buffer *const buffers[] = { buffer.p };
	set_buffer(context, static_cast<uint32_t>(slot), 1, buffers);
}

void constant_buffer::update(context_t context, std::size_t new_size, const void *buffer_data)
{
	assert(buffer_size >= new_size);

	auto gpu_buffer = D3D11_MAPPED_SUBRESOURCE{};
	auto hr = context->Map(buffer.p,
	                       NULL,
	                       D3D11_MAP_WRITE_DISCARD,
	                       NULL,
	                       &gpu_buffer);
	assert(SUCCEEDED(hr));

	std::memcpy(gpu_buffer.pData, buffer_data, new_size);

	context->Unmap(buffer.p, NULL);
}

void constant_buffer::create_set_function()
{
	switch (stage)
	{
		case shader_stage::vertex:
			set_buffer = [](context_t context, uint32_t slot, uint32_t count, ID3D11Buffer *const *buffers)
			{
				context->VSSetConstantBuffers(slot, count, buffers);
			};
			break;
		case shader_stage::pixel:
			set_buffer = [](context_t context, uint32_t slot, uint32_t count, ID3D11Buffer *const *buffers)
			{
				context->PSSetConstantBuffers(slot, count, buffers);
			};
			break;
	}
}
#pragma endregion

#pragma region Shader Resource
shader_resource::shader_resource(device_t device, shader_stage stage_, shader_slot slot_, const std::vector<uint8_t> &data) :
	stage{ stage_ }, slot{ slot_ }
{
	auto hr = DirectX::CreateDDSTextureFromMemory(device,
	                                              data.data(),
	                                              data.size(),
	                                              &resource, &resource_view);
	assert(SUCCEEDED(hr));
	
	create_set_function();
}

shader_resource::shader_resource(direct3d11::device_t device, shader_stage stage_, shader_slot slot_, const CComPtr<ID3D11Texture2D> texture) :
	resource{ texture }, stage{ stage_ }, slot{ slot_ }
{
	auto hr = device->CreateShaderResourceView(resource, nullptr, &resource_view);
	assert(SUCCEEDED(hr));

	create_set_function();
}

shader_resource::~shader_resource() = default;

void shader_resource::activate(context_t context)
{
	ID3D11ShaderResourceView *const views[] = { resource_view.p };
	set_resource(context, static_cast<uint32_t>(slot), 1, views);
}

auto shader_resource::get_dxgi_surface() const -> CComPtr<IDXGISurface1>
{
	auto dxgi_surface = CComPtr<IDXGISurface1>{};
	auto hr = resource->QueryInterface<IDXGISurface1>(&dxgi_surface);
	assert(SUCCEEDED(hr));

	return dxgi_surface;
}

void shader_resource::create_set_function()
{
	switch (stage)
	{
		case shader_stage::vertex:
			set_resource = [](context_t context, uint32_t slot, uint32_t count, ID3D11ShaderResourceView *const *views)
			{
				context->VSSetShaderResources(slot, count, views);
			};
			break;
		case shader_stage::pixel:
			set_resource = [](context_t context, uint32_t slot, uint32_t count, ID3D11ShaderResourceView *const *views)
			{
				context->PSSetShaderResources(slot, count, views);
			};
			break;
	}
}
#pragma endregion