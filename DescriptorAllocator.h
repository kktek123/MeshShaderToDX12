#pragma once
#include "Util/IndexCreator.h"

class CIndexCreator;
class DescriptorAllocator
{
public:
	static DescriptorAllocator* Get();

	static void Create(DWORD dwMaxCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);
	static void Delete();
	DescriptorAllocator();
	~DescriptorAllocator();
private:

	static DescriptorAllocator* instance;

	ID3D12Device5* m_pD3DDevice = nullptr;
	ID3D12DescriptorHeap* m_pHeap = nullptr;
	CIndexCreator			m_IndexCreator;
	UINT					m_DescriptorSize = 0;

	void	Cleanup();
public:
	BOOL	Initialize(DWORD dwMaxCount, D3D12_DESCRIPTOR_HEAP_FLAGS Flags);
	BOOL	AllocDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCPUHandle);
	void	FreeDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle);
	BOOL	Check(D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle);
	D3D12_GPU_DESCRIPTOR_HANDLE	GetGPUHandleFromCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
	ID3D12DescriptorHeap* GetDescriptorHeap() { return m_pHeap; }

};

