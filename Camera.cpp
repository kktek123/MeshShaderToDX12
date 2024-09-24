#include "pch.h"
#include "D3D.h"
#include "Camera.h"

Camera::Camera()
{
	Rotation();
	Move();

}

Camera::~Camera()
{
}

void Camera::Update()
{
}

void Camera::Position(float x, float y, float z)
{
	Position(Vector3(x, y, z));
}

void Camera::Position(Vector3& vec)
{
	position = vec;

	Move();
}

void Camera::Position(Vector3* vec)
{
	*vec = position;
}

void Camera::Rotation(float x, float y, float z)
{
	Rotation(Vector3(x, y, z));
}

void Camera::Rotation(Vector3& vec)
{
	rotation = vec;

	Rotation();
}

void Camera::Rotation(Vector3* vec)
{
	*vec = rotation;
}

void Camera::RotationDegree(float x, float y, float z)
{
	RotationDegree(Vector3(x, y, z));
}

void Camera::RotationDegree(Vector3& vec)
{
	//rotation = vec * Math::PI / 180.0f;
	rotation = vec * 0.01745328f;

	Rotation();
}

void Camera::RotationDegree(Vector3* vec)
{
	//*vec = rotation * 180.0f / Math::PI;
	*vec = rotation * 57.29577957f;
}

Matrix Camera::GetMatrix()
{
	return matView;
}

void Camera::Rotation()
{
	Matrix X, Y, Z;
	X = Matrix::CreateRotationX(rotation.x);
	Y = Matrix::CreateRotationY(rotation.y);
	Z = Matrix::CreateRotationZ(rotation.z);

	matRotation = X * Y * Z;
	forward = Vector3::TransformNormal(Vector3(0, 0, 1), matRotation);
	up = Vector3::TransformNormal(Vector3(0, 1, 0), matRotation);
	right = Vector3::TransformNormal(Vector3(1, 0, 0), matRotation);
}

void Camera::Move()
{
	View();
}

void Camera::View()
{
	XMStoreFloat4x4(&matView, XMMatrixLookAtLH(XMLoadFloat3(&position), XMLoadFloat3(&(position + forward)), XMLoadFloat3(&up)));
}
