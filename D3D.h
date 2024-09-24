#pragma once
#include "pch.h"
#include <d3d12.h>

const UINT SWAP_CHAIN_FRAME_COUNT = 3;
const UINT MAX_PENDING_FRAME_COUNT = SWAP_CHAIN_FRAME_COUNT - 1;

struct D3DDesc
{
	wstring AppName;
	HINSTANCE Instance;
	HWND Handle;
	D3D12_VIEWPORT	m_Viewport = {};
	D3D12_RECT		m_ScissorRect = {};
	DWORD			Width = 0;
	DWORD			Height = 0;
	float			fNear = 0;
	float			fFar = 0;
	bool bVsync;
	bool bFullScreen;
	XMFLOAT4 Background;
	float 			m_fDPI = 96.0f;
};


class ResourceManager;
class DescriptorAllocator;
class CObject;

class D3D
{
public:
	static D3D* Get();

	static void Create(BOOL bEnableDebugLayer, BOOL bEnableGBV);
	static void Delete();

	static ID3D12Device5* GetDevice()
	{
		return D3DDevice;
	}
	Vector3 CheckPosition;

	static bool bRaytracingRender;

	ID3D12CommandQueue* GetCommandQueue()
	{
		return m_pCommandQueue;
	}

	IDXGISwapChain3* GetSwapChain()
	{
		return m_pSwapChain;
	}

	ResourceManager* GetResourceManager() 
	{ 
		return m_pResourceManager; 
	}

	UINT INL_GetSrvDescriptorSize() { return m_srvDescriptorSize; }
	DescriptorAllocator* INL_GetSingleDescriptorAllocator() { return m_pSingleDescriptorAllocator; }


	ID3D12GraphicsCommandList6* GetCurrentCommandList()
	{ return m_ppCommandList[m_dwCurContextIndex]; }
	ID3D12CommandAllocator* GetCommandAllocator()
	{ return m_ppCommandAllocator[m_dwCurContextIndex]; }

	
	ID3D12Resource* CurrentBackBuffer()const
	{
		return m_pRenderTargets[m_uiRenderTargetIndex];
	}

	DWORD FrameCount()
	{
		return m_dwCurContextIndex;
	}

	DXGI_FORMAT GetBackBufferFormat()
	{
		return m_backBufferFormat;
	}

	static float Width()
	{
		return d3dDesc.Width;
	}

	static float Height()
	{
		return d3dDesc.Height;
	}

	static const D3DDesc& GetDesc()
	{
		return d3dDesc;
	}

	static const HWND& GetHandle()
	{
		return d3dDesc.Handle;
	}

	static void SetDesc(D3DDesc& desc)
	{
		d3dDesc = desc;
	}



	void	BeginRender();
	void	EndRender();
	void	Present();
	BOOL	UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight);
	void	SetRenderTarget(ID3D12DescriptorHeap* rtvHeap, ID3D12DescriptorHeap* dsvHeap,DWORD dwThreadIndex = 0);
private:


	ID3D12PipelineState* m_pipelineState;
	ID3D12RootSignature* m_rootSignature;

	static D3D* instance;

	static D3DDesc d3dDesc;

	static const UINT MAX_DRAW_COUNT_PER_FRAME = 4096;
	static const UINT MAX_DESCRIPTOR_COUNT = 4096;
	static const UINT MAX_RENDER_THREAD_COUNT = 8;

	static const UINT DESCRIPTOR_COUNT_PER_OBJ = 1;			// | Constant Buffer
	static const UINT DESCRIPTOR_COUNT_PER_TRI_GROUP = 1;	// | SRV(tex)
	static const UINT MAX_TRI_GROUP_COUNT_PER_OBJ = 8;
	static const UINT MAX_DESCRIPTOR_COUNT_FOR_DRAW = DESCRIPTOR_COUNT_PER_OBJ + (MAX_TRI_GROUP_COUNT_PER_OBJ * DESCRIPTOR_COUNT_PER_TRI_GROUP);


	static ID3D12Device5* D3DDevice;
	ID3D12CommandQueue* m_pCommandQueue = nullptr;
	IDXGISwapChain3* m_pSwapChain = nullptr;
	ResourceManager* m_pResourceManager = nullptr;
	ID3D12CommandAllocator* m_ppCommandAllocator[MAX_PENDING_FRAME_COUNT] = {};
	ID3D12GraphicsCommandList6* m_ppCommandList[MAX_PENDING_FRAME_COUNT] = {};

	DXGI_ADAPTER_DESC1	m_AdapterDesc = {};
	DXGI_FORMAT                                         m_backBufferFormat;


	DescriptorAllocator* m_pSingleDescriptorAllocator = nullptr;
	

	HANDLE	m_hFenceEvent = nullptr;
	ID3D12Fence* m_pFence = nullptr;
	UINT64	m_pui64LastFenceValue[MAX_PENDING_FRAME_COUNT] = {};
	UINT64	m_ui64FenceValue = 0;


	ID3D12Resource* m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT] = {};
	ID3D12Resource* m_pDepthStencil = nullptr;
	ID3D12DescriptorHeap* m_pRTVHeap = nullptr;
	ID3D12DescriptorHeap* m_pDSVHeap = nullptr;
	ID3D12DescriptorHeap* m_pSRVHeap = nullptr;
	ID3D12DescriptorHeap* m_pImguiHeap = nullptr;

	UINT	m_rtvDescriptorSize = 0;
	UINT	m_srvDescriptorSize = 0;
	UINT	m_dsvDescriptorSize = 0;
	UINT	m_dwSwapChainFlags = 0;
	UINT	m_uiRenderTargetIndex = 0;


	DWORD	m_dwCurContextIndex = 0;
	DWORD m_dwCurThreadIndex = 0;
	DWORD m_dwRenderThreadCount = 0;

private:
	D3D();
	~D3D();

	BOOL	Initialize(BOOL bEnableDebugLayer, BOOL bEnableGBV);

	BOOL	CreateDepthStencil(UINT Width, UINT Height);

	void	CreateFence();
	void	CleanupFence();
	void	CreateCommandList();
	void	CleanupCommandList();
	BOOL	CreateDescriptorHeapForRTV();
	BOOL	CreateDescriptorHeapForDSV();
	BOOL	CreateDescriptorHeapForImgui();

	void	CleanupDescriptorHeapForRTV();
	void	CleanupDescriptorHeapForDSV();

	UINT64	Fence();
	void	WaitForFenceValue(UINT64 ExpectedFenceValue);

	void	Cleanup();
	void MoveToNextFrame();
public:
	void	ExecuteCommandList();
	void	WaitForGpu();
};

