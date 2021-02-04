#pragma once

#include <cstdlib>
#include <cmath>
#include <DirectXMath.h>

using DirectX::XMFLOAT2;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;
using DirectX::XMFLOAT4X4;
using DirectX::XMFLOAT3X3;

namespace CD {

using Vector2 = XMFLOAT2;
using Vector3 = XMFLOAT3;
using Vector4 = XMFLOAT4;

using Matrix4x4 = XMFLOAT4X4;
using Matrix3x3 = XMFLOAT3X3;

using Quaternion = XMFLOAT4;

struct Transform {
	Quaternion orientation;
	Vector3 position;
	Vector3 scale;
};

inline Transform transform_identity() {
	return {
		{0.f, 0.f, 0.f, 1.f},
		{0.f, 0.f, 0.f},
		{1.f, 1.f, 1.f}
	};
}

inline Matrix4x4 matrix_transform(const Transform& transform) {
	using namespace DirectX;

	XMVECTOR scale = XMLoadFloat3(&transform.scale);
	XMVECTOR orientation = XMLoadFloat4(&transform.orientation);
	XMVECTOR translation = XMLoadFloat3(&transform.position);

	XMMATRIX m = XMMatrixAffineTransformation(scale, {}, orientation, translation);

	Matrix4x4 ret;
	XMStoreFloat4x4(&ret, m);
	return ret;
}

inline Matrix4x4 matrix_perspective(float fovy, float aspect_ratio, float near_z, float far_z) {
	using namespace DirectX;

	XMMATRIX perspective = XMMatrixPerspectiveFovLH(fovy, aspect_ratio, near_z, far_z);
	Matrix4x4 ret;
	XMStoreFloat4x4(&ret, perspective);
	return ret;
}

inline Matrix4x4 matrix_orthographic(float width, float height, float near_z, float far_z) {
	using namespace DirectX;

	XMMATRIX orthographic = XMMatrixOrthographicLH(width, height, near_z, far_z);
	Matrix4x4 ret;
	XMStoreFloat4x4(&ret, orthographic);
	return ret;
}

inline void rotate(Vector3& u, const Quaternion& q) {
	using namespace DirectX;

	XMVECTOR vector = XMLoadFloat3(&u);
	XMVECTOR quaternion = XMLoadFloat4(&q);
	vector = XMVector3Rotate(vector, quaternion);
	XMStoreFloat3(&u, vector);
}

struct AABB {
	Vector3 min;
	Vector3 max;
};

}