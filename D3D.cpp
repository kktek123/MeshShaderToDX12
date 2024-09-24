#include "pch.h"
#include "ResourceManager.h"
#include "DescriptorAllocator.h"
#include "Util/ProcessorInfo.h"
#include "Context.h"
#include "D3D.h"
#include "DXSampleHelper.h"
#include "RaytracingUtil.h"
#include "imgui_internal.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

D3DDesc D3D::d3dDesc;
D3D* D3D::instance = nullptr;
ID3D12Device5* D3D::D3DDevice = nullptr;
bool D3D::bRaytracingRender = false;




D3D* D3D::Get()
{
	assert(instance != NULL);

	return instance;
}

void D3D::Create(BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
	assert(instance == nullptr);

	instance = new D3D();
	instance->Initialize(bEnableDebugLayer, bEnableGBV);
}

void D3D::Delete()
{
	if (instance)
	{
		delete instance;
		instance = nullptr;
	}
}

D3D::D3D()
{
}

BOOL D3D::Initialize(BOOL bEnableDebugLayer, BOOL bEnableGBV)
{
	BOOL	bResult = FALSE;

	HRESULT hr = S_OK;
	ID3D12Debug* pDebugController = nullptr;
	IDXGIFactory6* pFactory = nullptr;
	IDXGIAdapter1* pAdapter = nullptr;
	DXGI_ADAPTER_DESC1 AdapterDesc = {};
	IDXGIInfoQueue* dxgiInfoQueue;
		bool debugDXGI = false;

	DWORD dwCreateFlags = 0;
	DWORD dwCreateFactoryFlags = 0;

	// if use debug Layer...
	if (bEnableDebugLayer)
	{
		// Enable the D3D12 debug layer.
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
		{
			pDebugController->EnableDebugLayer();
		}
		dwCreateFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		if (bEnableGBV)
		{
			ID3D12Debug5* pDebugController5 = nullptr;
			if (S_OK == pDebugController->QueryInterface(IID_PPV_ARGS(&pDebugController5)))
			{
				pDebugController5->SetEnableGPUBasedValidation(TRUE);
				pDebugController5->SetEnableAutoName(TRUE);
				pDebugController5->Release();
			}
		}
	}

	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
	{
		debugDXGI = true;
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

	}

	// Create DXGIFactory
	ThrowIfFailed(CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(&pFactory)));

	BOOL allowTearing = FALSE;

	//ThrowIfFailed(pFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)));



	D3D_FEATURE_LEVEL	featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	DWORD	FeatureLevelNum = _countof(featureLevels);

	for (DWORD featerLevelIndex = 0; featerLevelIndex < FeatureLevelNum; featerLevelIndex++)
	{
		UINT adapterIndex = 0;
		while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter))
		{
			pAdapter->GetDesc1(&AdapterDesc);

			if (SUCCEEDED(D3D12CreateDevice(pAdapter, featureLevels[featerLevelIndex], IID_PPV_ARGS(&D3DDevice))))
			{
				goto lb_exit;

			}
			//ThrowIfFalse(IsDirectXRaytracingSupported(pAdapter));

			pAdapter->Release();
			pAdapter = nullptr;
			adapterIndex++;
		}
	}
lb_exit:

	if (!D3DDevice)
	{
		__debugbreak();
		//goto lb_return;
	}

	m_AdapterDesc = AdapterDesc;


	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
	if (FAILED(D3DDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
		|| (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_5))
	{
		OutputDebugStringA("ERROR: Shader Model 6.5 is not supported\n");
		throw std::exception("Shader Model 6.5 is not supported");
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
	if (FAILED(D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features)))
		|| (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
	{
		OutputDebugStringA("ERROR: Mesh Shaders aren't supported!\n");
		throw std::exception("Mesh Shaders aren't supported!");
	}

	if (pDebugController)
	{
		ID3D12InfoQueue* pInfoQueue = nullptr;
		D3DDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
		if (pInfoQueue)
		{
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

			D3D12_MESSAGE_ID hide[] =
			{
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
				// Workarounds for debug layer issues on hybrid-graphics systems
				D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
				D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
			};
			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = (UINT)_countof(hide);
			filter.DenyList.pIDList = hide;
			pInfoQueue->AddStorageFilterEntries(&filter);

			pInfoQueue->Release();
			pInfoQueue = nullptr;
		}
	}
	// Describe and create the command queue.
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		hr = D3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue));
		if (FAILED(hr))
		{
			__debugbreak();
			goto lb_return;
		}
	}

	CreateDescriptorHeapForRTV();

	m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	// Describe and create the swap chain.
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = SWAP_CHAIN_FRAME_COUNT;
		swapChainDesc.Width = d3dDesc.Width;
		swapChainDesc.Height = d3dDesc.Height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		//Raytracing
		swapChainDesc.Format = m_backBufferFormat;

		//swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
		//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		m_dwSwapChainFlags = swapChainDesc.Flags;


		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		IDXGISwapChain1* pSwapChain1 = nullptr;
		if (FAILED(pFactory->CreateSwapChainForHwnd(m_pCommandQueue, 
			d3dDesc.Handle, &swapChainDesc, &fsSwapChainDesc, nullptr, &pSwapChain1)))
		{
			__debugbreak();
		}
		pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
		pSwapChain1->Release();
		pSwapChain1 = nullptr;
		m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	}


	// Create frame resources.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	// Descriptor Table
	// |        0        |        1	       |
	// | Render Target 0 | Render Target 1 |
	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_pRenderTargets[n]));
		D3DDevice->CreateRenderTargetView(m_pRenderTargets[n], nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);

	}
	m_srvDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	// Create Depth Stencile resources
	CreateDescriptorHeapForDSV();
	CreateDepthStencil(d3dDesc.Width, d3dDesc.Height);


	// Create synchronization objects.
	CreateFence();

//	DWORD dwPhysicalCoreCount = 1;
//	DWORD dwLogicalCoreCount = 0;
//#ifdef USE_MULTI_THREAD
//#endif
//	GetPhysicalCoreCount(&dwPhysicalCoreCount, &dwLogicalCoreCount);
//
//	m_dwRenderThreadCount = dwPhysicalCoreCount;
//	if (m_dwRenderThreadCount > MAX_RENDER_THREAD_COUNT)
//		m_dwRenderThreadCount = MAX_RENDER_THREAD_COUNT;

	m_pResourceManager = new ResourceManager;
	m_pResourceManager->Initialize(D3DDevice);
	m_pSingleDescriptorAllocator = new DescriptorAllocator();
	m_pSingleDescriptorAllocator->Initialize(4096, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	if (Context::Get())
		Context::Get()->ResizeScreen();

	CreateCommandList();


	CreateDescriptorHeapForImgui();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	auto Heap = m_pSingleDescriptorAllocator->GetDescriptorHeap();
	ImGui_ImplWin32_Init(D3D::GetDesc().Handle);
	ImGui_ImplDX12_Init(D3DDevice, SWAP_CHAIN_FRAME_COUNT, m_backBufferFormat, m_pImguiHeap, m_pImguiHeap->GetCPUDescriptorHandleForHeapStart(), m_pImguiHeap->GetGPUDescriptorHandleForHeapStart());


lb_return:
	if (pDebugController)
	{
		pDebugController->Release();
		pDebugController = nullptr;
	}
	if (pAdapter)
	{
		pAdapter->Release();
		pAdapter = nullptr;
	}
	if (pFactory)
	{
		pFactory->Release();
		pFactory = nullptr;
	}
	return bResult;
}

void D3D::CreateCommandList()
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		ID3D12CommandAllocator* pCommandAllocator = nullptr;
		ID3D12GraphicsCommandList6* pCommandList = nullptr;

		if (FAILED(D3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator))))
		{
			__debugbreak();
		}

		// Create the command list.
		if (FAILED(D3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList))))
		{
			__debugbreak();
		}

		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		pCommandList->Close();

		m_ppCommandAllocator[i] = pCommandAllocator;
		m_ppCommandList[i] = pCommandList;
	}
}

void D3D::CleanupCommandList()
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		ID3D12CommandAllocator* pCommandAllocator = m_ppCommandAllocator[i];
		ID3D12GraphicsCommandList6* pCommandList = m_ppCommandList[i];

		if (pCommandList)
		{
			ULONG ref_count = pCommandList->Release();
			pCommandList = nullptr;
		}
		if (pCommandAllocator)
		{
			ULONG ref_count = pCommandAllocator->Release();
			pCommandAllocator = nullptr;
		}
		m_ppCommandAllocator[i] = nullptr;
		m_ppCommandList[i] = nullptr;
	}
}

void D3D::CreateFence()
{
	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	if (FAILED(D3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence))))
	{
		__debugbreak();
	}

	m_ui64FenceValue = 0;

	// Create an event handle to use for frame synchronization.
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

}



void D3D::BeginRender()
{
	Fence();
	ID3D12GraphicsCommandList6* m_commandList = m_ppCommandList[m_dwCurContextIndex];

	ThrowIfFailed(m_ppCommandAllocator[m_dwCurContextIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandList->Reset(m_ppCommandAllocator[m_dwCurContextIndex], m_pipelineState));

	// Set necessary state.
	m_commandList->RSSetViewports(1, &d3dDesc.m_Viewport);
	m_commandList->RSSetScissorRects(1, &d3dDesc.m_ScissorRect);

	SetRenderTarget(NULL, NULL);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

	// Indicate that the back buffer will be used as a render target.
	const auto toRenderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex],
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_commandList->ResourceBarrier(1, &toRenderTargetBarrier);


	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	





}

void D3D::EndRender()
{
	ID3D12GraphicsCommandList6* m_commandList = m_ppCommandList[m_dwCurContextIndex];
	m_commandList->SetDescriptorHeaps(1, &m_pImguiHeap);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1u, &rtvCPUHandle, FALSE, nullptr);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList);

	D3D12_RESOURCE_BARRIER postImguiBarriers[1] = {};
	postImguiBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pRenderTargets[m_uiRenderTargetIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COMMON
	);
	m_commandList->ResourceBarrier(ARRAYSIZE(postImguiBarriers), postImguiBarriers);


	// Indicate that the back buffer will now be used to present.
	const auto toPresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &toPresentBarrier);

	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void D3D::Present()
{
	Fence();

	//
	// Back Buffer 화면을 Primary Buffer로 전송
	//
	//UINT m_SyncInterval = 1;	// VSync On
	UINT m_SyncInterval = 0;	// VSync Off

	UINT uiSyncInterval = m_SyncInterval;
	UINT uiPresentFlags = 0;

	if (!uiSyncInterval)
	{
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}

	HRESULT hr = m_pSwapChain->Present(uiSyncInterval, uiPresentFlags);

	if (DXGI_ERROR_DEVICE_REMOVED == hr)
	{
		//__debugbreak();
	}

	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// prepare next frame
	DWORD	dwNextContextIndex = (m_dwCurContextIndex + 1) % MAX_PENDING_FRAME_COUNT;
	WaitForFenceValue(m_pui64LastFenceValue[dwNextContextIndex]);

	m_dwCurContextIndex = dwNextContextIndex;

}

UINT64 D3D::Fence()
{
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence, m_ui64FenceValue);
	m_pui64LastFenceValue[m_dwCurContextIndex] = m_ui64FenceValue;
	return m_ui64FenceValue;
}


void D3D::SetRenderTarget(ID3D12DescriptorHeap* rtvHeap, ID3D12DescriptorHeap* dsvHeap, DWORD dwThreadIndex)
{
	if (rtvHeap == nullptr)
		rtvHeap = m_pRTVHeap;
	if (dsvHeap == nullptr)
		dsvHeap = m_pDSVHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());

	ID3D12GraphicsCommandList6* pCommandList = m_ppCommandList[m_dwCurContextIndex];

	pCommandList->RSSetViewports(1, &d3dDesc.m_Viewport);
	pCommandList->RSSetScissorRects(1, &d3dDesc.m_ScissorRect);
	pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	pCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}


BOOL D3D::CreateDescriptorHeapForRTV()
{
	HRESULT hr = S_OK;

	// 렌더타겟용 디스크립터힙
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_FRAME_COUNT;	// SwapChain Buffer 0	| SwapChain Buffer 1
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(D3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap))))
	{
		__debugbreak();
	}

	m_rtvDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	return TRUE;
}

BOOL D3D::CreateDescriptorHeapForDSV()
{
	HRESULT hr = S_OK;

	// Describe and create a depth stencil view (DSV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;	// Default Depth Buffer
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(D3DDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDSVHeap))))
	{
		__debugbreak();
	}

	m_dsvDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	return TRUE;
}

BOOL D3D::CreateDescriptorHeapForImgui()
{
	HRESULT hr = S_OK;

	// Describe and create a depth stencil view (DSV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; //CBV,SRV,UAV는 Shader에서 참조해야함
	heapDesc.NodeMask = 0u;
	if (FAILED(D3DDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pImguiHeap))))
	{
		__debugbreak();
	}

	m_dsvDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return TRUE;
}

void D3D::WaitForFenceValue(UINT64 ExpectedFenceValue)
{
	// Wait until the previous frame is finished.
	if (m_pFence->GetCompletedValue() < ExpectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObjectEx(m_hFenceEvent, INFINITE , false);
	}
}


BOOL D3D::CreateDepthStencil(UINT Width, UINT Height)
{
	// Create the depth stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_RESOURCE_DESC depthDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		Width,
		Height,
		1,
		1,
		DXGI_FORMAT_R32_TYPELESS,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	if (FAILED(D3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_pDepthStencil)
	)))
	{
		__debugbreak();
	}
	m_pDepthStencil->SetName(L"CD3D12Renderer::m_pDepthStencil");

	CD3DX12_CPU_DESCRIPTOR_HANDLE	dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
	D3DDevice->CreateDepthStencilView(m_pDepthStencil, &depthStencilDesc, dsvHandle);

	return TRUE;
}

BOOL D3D::UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight)
{
	BOOL	bResult = FALSE;

	if (!(dwBackBufferWidth * dwBackBufferHeight))
		return FALSE;

	if (d3dDesc.Width == dwBackBufferWidth && d3dDesc.Height == dwBackBufferHeight)
		return FALSE;

	// wait for all commands
	Fence();

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	DXGI_SWAP_CHAIN_DESC1	desc;
	HRESULT	hr = m_pSwapChain->GetDesc1(&desc);
	if (FAILED(hr))
		__debugbreak();

	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pRenderTargets[n]->Release();
		m_pRenderTargets[n] = nullptr;
	}

	if (m_pDepthStencil)
	{
		m_pDepthStencil->Release();
		m_pDepthStencil = nullptr;
	}

	if (FAILED(m_pSwapChain->ResizeBuffers(SWAP_CHAIN_FRAME_COUNT, dwBackBufferWidth, dwBackBufferHeight, m_backBufferFormat, m_dwSwapChainFlags)))
	{
		__debugbreak();
	}


	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// Create frame resources.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_pRenderTargets[n]));
		D3DDevice->CreateRenderTargetView(m_pRenderTargets[n], nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	CreateDepthStencil(dwBackBufferWidth, dwBackBufferHeight);

	d3dDesc.Width = dwBackBufferWidth;
	d3dDesc.Height = dwBackBufferHeight;
	d3dDesc.m_Viewport.Width = (float)d3dDesc.Width;
	d3dDesc.m_Viewport.Height = (float)d3dDesc.Height;
	d3dDesc.m_ScissorRect.left = 0;
	d3dDesc.m_ScissorRect.top = 0;
	d3dDesc.m_ScissorRect.right = d3dDesc.Width;
	d3dDesc.m_ScissorRect.bottom = d3dDesc.Height;
	if(Context::Get())
		Context::Get()->ResizeScreen();
	//InitCamera();

	return TRUE;
}

void D3D::CleanupFence()
{
	if (m_hFenceEvent)
	{
		CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
	if (m_pFence)
	{
		m_pFence->Release();
		m_pFence = nullptr;
	}
}

void D3D::CleanupDescriptorHeapForRTV()
{
	if (m_pRTVHeap)
	{
		m_pRTVHeap->Release();
		m_pRTVHeap = nullptr;
	}
}


void D3D::CleanupDescriptorHeapForDSV()
{
	if (m_pDSVHeap)
	{
		m_pDSVHeap->Release();
		m_pDSVHeap = nullptr;
	}
}

void D3D::Cleanup()
{

	Fence();

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_pui64LastFenceValue[i]);
	}

	if (m_pResourceManager)
	{
		delete m_pResourceManager;
		m_pResourceManager = nullptr;
	}
	if (m_pSingleDescriptorAllocator)
	{
		delete m_pSingleDescriptorAllocator;
		m_pSingleDescriptorAllocator = nullptr;
	}

	CleanupDescriptorHeapForRTV();
	CleanupDescriptorHeapForDSV();
	if (m_pImguiHeap)
	{
		m_pImguiHeap->Release();
		m_pImguiHeap = nullptr;
	}
	for (DWORD i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++)
	{
		if (m_pRenderTargets[i])
		{
			m_pRenderTargets[i]->Release();
			m_pRenderTargets[i] = nullptr;
		}
	}
	if (m_pDepthStencil)
	{
		ULONG ref_count = m_pDepthStencil->Release();
		m_pDepthStencil = nullptr;
	}
	if (m_pSwapChain)
	{
		ULONG ref_count = m_pSwapChain->Release();
		m_pSwapChain = nullptr;
	}

	if (m_pCommandQueue)
	{
		ULONG ref_count = m_pCommandQueue->Release();
		m_pCommandQueue = nullptr;
	}

	CleanupCommandList();

	CleanupFence();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (D3DDevice)
	{
		ULONG ref_count = D3DDevice->Release();
		if (ref_count)
		{
			//resource leak!!!
			IDXGIDebug1* pDebug = nullptr;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
			{
				pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
				pDebug->Release();
			}
			__debugbreak();
		}

		D3DDevice = nullptr;

	}
}

void D3D::MoveToNextFrame()
{
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = m_pui64LastFenceValue[m_dwCurContextIndex];
	ThrowIfFailed(m_pCommandQueue->Signal(m_pFence, currentFenceValue));

	// Update the frame index.
	m_dwCurContextIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_pFence->GetCompletedValue() < m_pui64LastFenceValue[m_dwCurContextIndex])
	{
		ThrowIfFailed(m_pFence->SetEventOnCompletion(m_pui64LastFenceValue[m_dwCurContextIndex], m_hFenceEvent));
		WaitForSingleObjectEx(m_hFenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	m_pui64LastFenceValue[m_dwCurContextIndex] = currentFenceValue + 1;
}

void D3D::ExecuteCommandList()
{
	ID3D12GraphicsCommandList6* m_commandList = m_ppCommandList[m_dwCurContextIndex];

	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void D3D::WaitForGpu()
{
	ThrowIfFailed(m_pCommandQueue->Signal(m_pFence, m_pui64LastFenceValue[m_dwCurContextIndex]));

	// Wait until the fence has been processed.
	ThrowIfFailed(m_pFence->SetEventOnCompletion(m_pui64LastFenceValue[m_dwCurContextIndex], m_hFenceEvent));
	WaitForSingleObjectEx(m_hFenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	m_pui64LastFenceValue[m_dwCurContextIndex]++;
}





D3D::~D3D()
{
	Cleanup();
}

