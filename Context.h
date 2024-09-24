#pragma once
#include "Freedom.h"
class Context
{
public:
	static Context* Get();
	static void Create();
	static void Delete();

private:
	Context();
	~Context();

public:
	void ResizeScreen();

	void Update();
	void Render();

	Matrix View();
	Matrix Projection();

	Matrix GetPerspective() { return perspective; }
	Viewport* GetViewport() { return viewport; }
	class Camera* GetCamera() { return camera; }

	Color& Ambient() { return ambient; }
	Color& Specular() { return specular; }
	Vector3& Direction() { return direction; }
	Vector3& Position() { return position; }
	Vector3 Rotation() { return camera->GetRotation(); }

	void SetCommandList(ID3D12GraphicsCommandList6* myCommandList);

private:
	static Context* instance;

private:
	Matrix perspective;
	Viewport* viewport;
	class Camera* camera;

	Color ambient = Color(0, 0, 0, 1);
	Color specular = Color(1, 1, 1, 1);
	Vector3 direction = Vector3(-1, -1, 1);
	Vector3 position = Vector3(0, 0, 0);
	ID3D12GraphicsCommandList6* CommandList;
};

