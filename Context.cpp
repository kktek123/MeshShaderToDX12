#include "pch.h"
#include "D3D.h"
#include "Freedom.h"
#include "Context.h"

Context* Context::instance = NULL;


Context* Context::Get()
{
	//assert(instance != NULL);

	return instance;
}

void Context::Create()
{
	assert(instance == NULL);

	instance = new Context();

}

void Context::Delete()
{
	SafeDelete(instance);
}

Context::Context()
{
	D3DDesc desc = D3D::GetDesc();
	float fAspectRatio = (float)desc.Width / (float)desc.Height;

	//perspective = Matrix::CreatePerspective(desc.Width, desc.Height, desc.fNear, desc.fFar);
	XMStoreFloat4x4(&perspective, XMMatrixPerspectiveFovLH(XM_PI / 3.0f, fAspectRatio, desc.fNear, desc.fFar));
	
	viewport = new Viewport(desc.m_Viewport);
	camera = new Freedom();
	//camera->Position(Vector3(0, 150, 150));
}

Context::~Context()
{
	SafeDelete(viewport);
	SafeDelete(camera);
}

void Context::ResizeScreen()
{
	D3DDesc desc = D3D::GetDesc();
	float fAspectRatio = (float)desc.Width / (float)desc.Height;
	//float fovAngle = XMConvertToRadians(45.0f); //XM_PI / 3.0f
	XMStoreFloat4x4(&perspective, XMMatrixPerspectiveFovLH(XM_PI / 3.0f, fAspectRatio, desc.fNear, desc.fFar));
	//CommandList->RSSetViewports(1, viewport->Get12());
}

void Context::Update()
{
	camera->Update();
}

void Context::Render()
{
	CommandList->RSSetViewports(1, viewport->Get12());

	//string str = string("Frame Rate : ") + to_string(ImGui::GetIO().Framerate);
	//Gui::Get()->RenderText(5, 5, 1, 1, 1, str);

	Vector3 camPos;
	camera->Position(&camPos);

	//Vector3 camDir;
	//camera->RotationDegree(&camDir);

	//str = "Cam Position : ";
	//str += to_string((int)camPos.x) + ", " + to_string((int)camPos.y) + ", " + to_string((int)camPos.z);
	//Gui::Get()->RenderText(5, 20, 1, 1, 1, str);

	//str = "Cam Rotation : ";
	//str += to_string((int)camDir.x) + ", " + to_string((int)camDir.y);
	//Gui::Get()->RenderText(5, 35, 1, 1, 1, str);

}

Matrix Context::View()
{
	return camera->GetMatrix();
}

Matrix Context::Projection()
{
	return perspective;
}

void Context::SetCommandList(ID3D12GraphicsCommandList6* myCommandList)
{
	CommandList = myCommandList;
}
