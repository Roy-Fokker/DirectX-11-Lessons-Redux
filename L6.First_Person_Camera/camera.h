#pragma once

#include <DirectXMath.h>
#include <array>

namespace dx11_lessons
{
	class camera
	{
	public:
		camera() = delete;
		camera(const DirectX::XMFLOAT3 &eye, const DirectX::XMFLOAT3 &focus, const DirectX::XMFLOAT3 &up);
		~camera();

		void translate(float dolly, float pan, float crane);
		void rotate(float roll, float pitch, float yaw);

		void look_at(const DirectX::XMFLOAT3 &eye, const DirectX::XMFLOAT3 &focus, const DirectX::XMFLOAT3 &up);

		[[nodiscard]]
		auto get_view() const -> DirectX::XMMATRIX;

		[[nodiscard]]
		auto get_position() const -> DirectX::XMVECTOR;

	private:
		DirectX::XMVECTOR position{};
		DirectX::XMVECTOR orientation{};
	};
}
