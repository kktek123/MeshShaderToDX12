#pragma once
#include "pch.h"

inline void AllocateUAVBuffer(ID3D12Device* pDevice, UINT64 bufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr)
{
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        initialResourceState,
        nullptr,
        IID_PPV_ARGS(ppResource)));
    if (resourceName)
    {
        (*ppResource)->SetName(resourceName);
    }
}

inline void AllocateUploadBuffer(ID3D12Device* pDevice, void* pData, UINT64 datasize, ID3D12Resource** ppResource, const wchar_t* resourceName = nullptr)
{
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppResource)));
    if (resourceName)
    {
        (*ppResource)->SetName(resourceName);
    }
    void* pMappedData;
    (*ppResource)->Map(0, nullptr, &pMappedData);
    memcpy(pMappedData, pData, datasize);
    (*ppResource)->Unmap(0, nullptr);
}

inline UINT Align(UINT size, UINT alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

// Returns bool whether the device supports DirectX Raytracing tier.


class GpuUploadBuffer
{
public:
    ID3D12Resource* GetResource() { return m_resource; }

protected:
    ID3D12Resource* m_resource;

    GpuUploadBuffer() {}
    ~GpuUploadBuffer()
    {
        if (m_resource)
        {
            m_resource->Unmap(0, nullptr);
            //m_resource->Release();
            //m_resource = nullptr;
        }
    }

    void Allocate(ID3D12Device* device, UINT bufferSize, LPCWSTR resourceName = nullptr)
    {
        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        ThrowIfFailed(device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_resource)));
        m_resource->SetName(resourceName);
    }

    uint8_t* MapCpuWriteOnly()
    {
        uint8_t* mappedData;
        // We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
        return mappedData;
    }
};

struct D3DBuffer
{
    ID3D12Resource* resource;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
};

//// Helper class to create and update a constant buffer with proper constant buffer alignments.
//// Usage: ToDo
////    ConstantBuffer<...> cb;
////    cb.Create(...);
////    cb.staging.var = ...; | cb->var = ... ; 
////    cb.CopyStagingToGPU(...);
//template <class T>
//class ConstantBuffer : public GpuUploadBuffer
//{
//    uint8_t* m_mappedConstantData;
//    UINT m_alignedInstanceSize;
//    UINT m_numInstances;
//
//public:
//    ConstantBuffer() : m_alignedInstanceSize(0), m_numInstances(0), m_mappedConstantData(nullptr) {}
//
//    void Create(ID3D12Device* device, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
//    {
//        m_numInstances = numInstances;
//        UINT alignedSize = Align(sizeof(T), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
//        UINT bufferSize = numInstances * alignedSize;
//        Allocate(device, bufferSize, resourceName);
//        m_mappedConstantData = MapCpuWriteOnly();
//    }
//
//    void CopyStagingToGpu(UINT instanceIndex = 0)
//    {
//        memcpy(m_mappedConstantData + instanceIndex * m_alignedInstanceSize, &staging, sizeof(T));
//    }
//
//    // Accessors
//    T staging;
//    T* operator->() { return &staging; }
//    UINT NumInstances() { return m_numInstances; }
//    D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0)
//    {
//        return m_resource->GetGPUVirtualAddress() + instanceIndex * m_alignedInstanceSize;
//    }
//};
//
//
//// Helper class to create and update a structured buffer.
//// Usage: ToDo
////    ConstantBuffer<...> cb;
////    cb.Create(...);
////    cb.staging.var = ...; | cb->var = ... ; 
////    cb.CopyStagingToGPU(...);
//template <class T>
//class StructuredBuffer : public GpuUploadBuffer
//{
//    T* m_mappedBuffers;
//    std::vector<T> m_staging;
//    UINT m_numInstances;
//
//public:
//    // Performance tip: Align structures on sizeof(float4) boundary.
//    // Ref: https://developer.nvidia.com/content/understanding-structured-buffer-performance
//    static_assert(sizeof(T) % 16 == 0, L"Align structure buffers on 16 byte boundary for performance reasons.");
//
//    StructuredBuffer() : m_mappedBuffers(nullptr), m_numInstances(0) {}
//
//    void Create(ID3D12Device* device, UINT numElements, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
//    {
//        m_staging.resize(numElements);
//        UINT bufferSize = numInstances * numElements * sizeof(T);
//        Allocate(device, bufferSize, resourceName);
//        m_mappedBuffers = reinterpret_cast<T*>(MapCpuWriteOnly());
//    }
//
//    void CopyStagingToGpu(UINT instanceIndex = 0)
//    {
//        memcpy(m_mappedBuffers + instanceIndex, &m_staging[0], InstanceSize());
//    }
//
//    // Accessors
//    T& operator[](UINT elementIndex) { return m_staging[elementIndex]; }
//    size_t NumElementsPerInstance() { return m_staging.size(); }
//    UINT NumInstances() { return m_staging.size(); }
//    size_t InstanceSize() { return NumElementsPerInstance() * sizeof(T); }
//    D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0)
//    {
//        return m_resource->GetGPUVirtualAddress() + instanceIndex * InstanceSize();
//    }
//};