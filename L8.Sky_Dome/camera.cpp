#include "camera.h"

using namespace dx11_lessons;
using namespace DirectX;

namespace
{
	const auto forward_vector = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	const auto left_vector    = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	const auto up_vector      = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	auto rotate_vector(const XMVECTOR &q, const XMVECTOR &v) -> XMVECTOR
	{
		auto rotated_vector = XMVectorSetW(v, 1.0f);
		auto conjugate_quat = XMQuaternionConjugate(q);

		rotated_vector = XMQuaternionMultiply(q, rotated_vector);
		rotated_vector = XMQuaternionMultiply(rotated_vector, conjugate_quat);

		return XMVector3Normalize(rotated_vector);
	}
}

camera::camera(const DirectX::XMFLOAT3 &eye, const DirectX::XMFLOAT3 &focus, const DirectX::XMFLOAT3 &up)
{
	look_at(eye, focus, up);
}

camera::~camera() = default;

void camera::translate(float dolly, float pan, float crane)
{
	auto forward = rotate_vector(orientation, forward_vector);
	auto left = rotate_vector(orientation, left_vector);
	auto up = rotate_vector(orientation, up_vector);

	position = (forward * dolly)
	         + (left    * pan)
	         + (up      * crane)
	         + position;
}

void camera::rotate(float roll, float pitch, float yaw)
{
	auto forward = rotate_vector(orientation, forward_vector);
	auto left = rotate_vector(orientation, left_vector);
	auto up = up_vector;  // rotate_vector(orientation, up_vector);

	auto rot_x = XMQuaternionRotationAxis(left, pitch);
	auto rot_y = XMQuaternionRotationAxis(up, yaw);
	auto rot_z = XMQuaternionRotationAxis(forward, roll);

	orientation = XMQuaternionMultiply(rot_x, orientation);
	orientation = XMQuaternionMultiply(rot_y, orientation);
	orientation = XMQuaternionMultiply(rot_z, orientation);
}

void camera::look_at(const DirectX::XMFLOAT3 & eye, const DirectX::XMFLOAT3 & focus, const DirectX::XMFLOAT3 & up)
{
	position = XMLoadFloat3(&eye);
	auto focus_direction = XMLoadFloat3(&focus),
	     up_direction = XMLoadFloat3(&up);

	auto view_matrix = XMMatrixLookAtLH(position, focus_direction, up_direction);

	orientation = XMQuaternionRotationMatrix(view_matrix);
}

auto camera::get_view() const -> DirectX::XMMATRIX
{
	auto translation = XMMatrixTranslationFromVector(-position),
		rotation = XMMatrixRotationQuaternion(orientation);

	return translation * rotation;
}

auto dx11_lessons::camera::get_position() const -> DirectX::XMVECTOR
{
	return position;
}
