#pragma once

#include <Windows.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <atlbase.h>

namespace dx11_lessons
{
	class direct3d11 final
	{
	public:
		using device_t = CComPtr<ID3D11Device>;
		using swapchain_t = CComPtr<IDXGISwapChain>;
		using context_t = CComPtr<ID3D11DeviceContext>;

		static constexpr auto swapchain_format = DXGI_FORMAT_R8G8B8A8_UNORM;

	public:
		direct3d11() = delete;
		direct3d11(HWND hWnd);
		~direct3d11();

		void resize();
		void present(bool vSync);

		auto get_device() const -> device_t;
		auto get_swapchain() const -> swapchain_t;
		auto get_context() const -> context_t;

	private:
		void make_device_and_context();
		void make_swapchain();

	private:
		HWND hWnd{};

		device_t device{};
		swapchain_t swapchain{};
		context_t context{};
	};

	auto get_supported_msaa_level(direct3d11::device_t device) -> const DXGI_SAMPLE_DESC;
}