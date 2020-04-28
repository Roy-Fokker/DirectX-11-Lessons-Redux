#pragma once

#include "direct3d11.h"

#include <array>

namespace dx11_lessons
{
	class render_pass
	{
		using device_t = direct3d11::device_t;
		using swapchain_t = direct3d11::swapchain_t;
		using context_t = direct3d11::context_t;
		using render_target_view_t = CComPtr<ID3D11RenderTargetView>;
		using depth_stencil_view_t = CComPtr<ID3D11DepthStencilView>;
		using texture_2d_t = CComPtr<ID3D11Texture2D>;

	public:
		render_pass() = delete;
		render_pass(device_t device, swapchain_t swapchain);
		~render_pass();

		void activate(context_t context);
		void clear(context_t context, const std::array<float, 4> &clear_color);

	private:
		void create_viewport(swapchain_t swapchain);
		void create_render_target_view(device_t device, swapchain_t swapchain);
		void create_depth_stencil_view(device_t device, swapchain_t swapchain);

	private:
		D3D11_VIEWPORT viewport;
		render_target_view_t render_target_view;
		texture_2d_t depth_stencil_buffer;
		depth_stencil_view_t depth_stencil_view;
	};
}