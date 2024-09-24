
#include "pch.h"
#include "D3D.h"
#include <DDSTextureLoader.h>
#include "ResourceManager.h"
#include "DescriptorAllocator.h"
#include "GeometryGenerator.h"
#include "ObjectTest3.h"
#include "LandObject.h"
#include "MeshletObjectTest.h"
#include "AmpObjectTest.h"
#include "Test.h"
#include "Texture.h"
#include "RaytracingObjectTest.h"
#include "imgui_impl_dx12.h"
#include "imgui_internal.h"


const DWORD GAME_OBJ_COUNT = 100;


BOOL Test::Initialize()
{

	ModelObject = new CObjectTest3;
	ModelObject->Init();
	ModelObject->SetTrasform(Vector3(4, 0, 0));
	m_dwInitRefCount++;

	AmpObject = new AmpObjectTest;
	AmpObject->Init();
	AmpObject->SetTrasform(Vector3(1, 0, 0));
	m_dwInitRefCount++;


	return m_dwInitRefCount;
}

void Test::Destroy()
{

	if (ModelObject)
	{
		ModelObject->Destroy();
		delete ModelObject;
		ModelObject = nullptr;
	}
	if (AmpObject)
	{
		AmpObject->Destroy();
		delete AmpObject;
		AmpObject = nullptr;
	}

	CTextures::Delete();
}

void Test::Update()
{
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Post Effects")) {
		int flag = 0;
		flag += ImGui::RadioButton("VertexShader", (int*)&mode, RenderMeshShader::VS);
		ImGui::SameLine();
		flag += ImGui::RadioButton("MeshlShader", (int*)&mode, RenderMeshShader::MS);

		ImGui::TreePop();

	}
	switch (mode)
	{
	case None:
		break;
	case VS:
		ModelObject->Update();
		break;
	case MS:
		AmpObject->Update();
		break;
	default:
		break;
	}

}

void Test::Render()
{
	switch (mode)
	{
	case None:
		break;
	case VS:
		ModelObject->Render();
		break;
	case MS:
		AmpObject->Render();
		break;
	default:
		break;
	}
}

void Test::PostRender()
{

}
