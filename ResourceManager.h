#pragma once

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();
	BOOL	Initialize(ID3D12Device5* pD3DDevice);
	HRESULT CreateUAVBuffer(UINT64 SizePerResource, ID3D12Resource** ppOutBuffer, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr);
	HRESULT CreateResourceBuffer(UINT SizePerResource, DWORD dwNum, ID3D12Resource** ppOutBuffer, void* pInitData);
	HRESULT CreateStaticVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer, void* pInitData);
	HRESULT CreateDynamicVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer,ID3D12Resource** ppUploadBuffer, void* pInitData);
	HRESULT UpdateVertexBuffer(UINT SizePerVertex, DWORD dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource* pVertexBuffer, ID3D12Resource* pUploadBuffer, void* pInitData);
	HRESULT CreateIndexBuffer(DWORD dwIndexNum, D3D12_INDEX_BUFFER_VIEW* pOutIndexBufferView, ID3D12Resource** ppOutBuffer, void* pInitData);
	void	UpdateTextureForWrite(ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource);
	BOOL	CreateTexture(ID3D12Resource** ppOutResource, UINT Width, UINT Height, DXGI_FORMAT format, const BYTE* pInitImage);
	BOOL	CreateTextureFromFile(ID3D12Resource** ppOutResource, D3D12_RESOURCE_DESC* pOutDesc, const WCHAR* wchFileName);
	BOOL	CreateTexturePair(ID3D12Resource** ppOutResource, ID3D12Resource** ppOutUploadBuffer, UINT Width, UINT Height, DXGI_FORMAT format);
	BOOL	CreateStagingTexture(ID3D12Resource** ppOutResource, UINT Width, UINT Height, DXGI_FORMAT format, const BYTE* pInitImage,const int mipLevels,const int arraySize);

	ID3D12CommandQueue* GetCommandQueue() { return m_pCommandQueue; }
	ID3D12CommandAllocator* GetCommandAllocator() { return m_pCommandAllocator; }
	ID3D12GraphicsCommandList6* GetCommandList() { return m_pCommandList; }

	UINT64	Fence();
	void	WaitForFenceValue();
private:

	void	CreateFence();
	void	CleanupFence();
	void	CreateCommandList();
	void	CleanupCommandList();

	void	Cleanup();
	ID3D12Device5* pD3DDevice = nullptr;
	ID3D12CommandQueue* m_pCommandQueue = nullptr;
	ID3D12CommandAllocator* m_pCommandAllocator = nullptr;
	ID3D12GraphicsCommandList6* m_pCommandList = nullptr;

	HANDLE	m_hFenceEvent = nullptr;
	ID3D12Fence* m_pFence = nullptr;
	UINT64	m_ui64FenceValue = 0;


};