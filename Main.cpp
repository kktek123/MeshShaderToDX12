// DirectX12Project2.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "Main.h"
#include "D3D.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "DescriptorAllocator.h"
#include "Context.h"
#include "Test.h"
//#include <Mouse.h>
#include <iostream>
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

// imgui_impl_win32.cpp에 정의된 메시지 처리 함수에 대한 전방 선언
// Vcpkg를 통해 IMGUI를 사용할 경우 빨간줄로 경고가 뜰 수 있음
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);


BOOL Main::Initialize()
{
	Push(new Test());

	return !executes.empty();
}

void Main::Ready()
{

}

void Main::Destroy()
{
	for (IExecute* exe : executes)
	{
		exe->Destroy();
		delete exe;
		exe = nullptr;
	}
}

void Main::Update()
{
	for (IExecute* exe : executes)
		exe->Update();
}

void Main::PreRender()
{
	for (IExecute* exe : executes)
		exe->PreRender();
}

void Main::Render()
{
	for (IExecute* exe : executes)
		exe->Render();
}

void Main::PostRender()
{
	for (IExecute* exe : executes)
		exe->PostRender();
}

void Main::ResizeScreen()
{
	for (IExecute* exe : executes)
		exe->ResizeScreen();
}

void Main::Push(IExecute* execute)
{
	executes.push_back(execute);

	execute->Initialize();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
WPARAM Run(IExecute* main);
D3D12MeshletRender* MeshletRender;

BOOL InitGUI() {

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	ImGui_ImplWin32_Init(D3D::GetDesc().Handle);

	return TRUE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR param, int command)
{

	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	ThrowIfFailed(hr);

	D3DDesc desc;
	desc.AppName = L"D3D Game";
	desc.Instance = hInstance;
	desc.bFullScreen = false;
	desc.bVsync = false;
	desc.Handle = NULL;
	desc.Width = 1280;
	desc.Height = 720;
	desc.fNear = 0.1f;
	desc.fFar = 1000;
	desc.Background = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	D3D::SetDesc(desc);
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = desc.Instance;
	wcex.hIcon = LoadIcon(desc.Instance, MAKEINTRESOURCE(IDI_MY01CREATEDEVICE));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY01CREATEDEVICE);
	wcex.lpszClassName = desc.AppName.c_str();
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	assert(RegisterClassExW(&wcex));

	HWND hWnd = CreateWindowW(wcex.lpszClassName, wcex.lpszClassName, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	desc.Handle = hWnd;
	RECT	rect;
	::GetClientRect(desc.Handle, &rect);
	desc.Width= rect.right - rect.left;
	desc.Height= rect.bottom - rect.top;


	desc.m_Viewport.Width = (float)desc.Width;
	desc.m_Viewport.Height = (float)desc.Height;
	desc.m_Viewport.MinDepth = 0.0f;
	desc.m_Viewport.MaxDepth = 1.0f;

	desc.m_ScissorRect.left = 0;
	desc.m_ScissorRect.top = 0;
	desc.m_ScissorRect.right = desc.Width;
	desc.m_ScissorRect.bottom = desc.Height;

	D3D::SetDesc(desc);

	if (!desc.Handle)
	{
		return FALSE;
	}


	UINT centerX = (GetSystemMetrics(SM_CXSCREEN) - (UINT)desc.Width) / 2;
	UINT centerY = (GetSystemMetrics(SM_CYSCREEN) - (UINT)desc.Height) / 2;

	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	MoveWindow
	(
		desc.Handle
		, centerX, centerY
		, rect.right - rect.left, rect.bottom - rect.top
		, TRUE
	);
	ShowWindow(desc.Handle, SW_SHOWNORMAL);
	SetForegroundWindow(desc.Handle);
	SetFocus(desc.Handle);
	ShowCursor(true);
	SetCursorPos(
		0,
		0);

	//InitGUI();


	Main* main = new Main();
	Context::Create();
	D3D::Create(FALSE, FALSE);
	Keyboard::Create();
	Mouse::Create();
	Mouse::Get()->SetHandle(desc.Handle);

	//MeshletRender = new D3D12MeshletRender(1280, 720, L"D3D12 Mesh Shader");
	//DescriptorAllocator::Create(4096, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	WPARAM wParam = Run(main);

	if (main)
	{
		delete main;
		main = nullptr;
	}
	return wParam;
}

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.

IExecute* mainExecute = NULL;

ULONGLONG m_PrvFrameCheckTick = 0;
ULONGLONG m_PrvUpdateTick = 0;
DWORD	m_FrameCount = 0;
DWORD	m_FPS = 0;
DWORD	m_dwCommandListCount = 0;




WPARAM Run(IExecute* main)
{
	mainExecute = main;

	mainExecute->Initialize();

	Context::Get()->SetCommandList(D3D::Get()->GetCurrentCommandList());

	MSG msg;
	while (true)

	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			//MainRender();
			{

				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();

				ImGui::NewFrame();
				ImGui::Begin("Scene Control");

				// ImGui가 측정해주는 Framerate 출력
				ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
					1000.0f / ImGui::GetIO().Framerate,
					ImGui::GetIO().Framerate);


				//ImGui::GetIO();
				m_FrameCount++;

				// begin
				ULONGLONG CurTick = GetTickCount64();
				if (Keyboard::Get()->Down('R'))
					D3D::bRaytracingRender = !D3D::bRaytracingRender;

				D3D::Get()->BeginRender();



				Keyboard::Get()->Update();
				Mouse::Get()->Update();

				Context::Get()->Update();
				Context::Get()->Render();

				mainExecute->Update();
				ImGui::End();
				ImGui::Render();
				mainExecute->Render();

				D3D::Get()->EndRender();
				mainExecute->PostRender();

				D3D::Get()->Present();



				POINT point;
				GetCursorPos(&point);
				ClientToScreen(D3D::GetDesc().Handle, &point);
			}
		}
	}
	mainExecute->Destroy();

	//DescriptorAllocator::Get()->Delete();
	D3D::Get()->Delete();
	Keyboard::Delete();
	Mouse::Delete();
	Context::Delete();

	//MeshletRender->OnDestroy();


	if (D3D::GetDesc().bFullScreen == true)
		ChangeDisplaySettings(NULL, 0);

	DestroyWindow(D3D::GetDesc().Handle);

	UnregisterClass(D3D::GetDesc().AppName.c_str(), D3D::GetDesc().Instance); 

#ifdef _DEBUG
	_ASSERT(_CrtCheckMemory());
#endif
	return msg.wParam;

}

void Create()
{
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Mouse::Get()->InputProc(message, wParam, lParam);
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	if (message == WM_SIZE)
	{
		if (mainExecute != NULL)
		{
			float width = (float)LOWORD(lParam);
			float height = (float)HIWORD(lParam);

			if (D3D::Get() != NULL)
				D3D::Get()->UpdateWindowSize(width, height);

			//if (Context::Get() != NULL)
			//	Context::Get()->ResizeScreen();

			mainExecute->ResizeScreen();
		}
	}

	//if (message == WM_MOUSEMOVE)
	//{
	//	Mouse::Get()->Update();
	//	InvalidateRect(hWnd, NULL, FALSE);
	//}
	if (message == WM_CLOSE || message == WM_DESTROY)
	{
		PostQuitMessage(0);

		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);

}