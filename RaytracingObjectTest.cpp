#include "pch.h"
#include "D3D.h"
#include "ResourceManager.h"
#include "DescriptorAllocator.h"
#include "Context.h"
#include "GeometryGenerator.h"
#include <DDSTextureLoader.h>
#include <fp16.h>
#include <assert.h>
#include "RaytracingObjectTest.h"
#include "Texture.h"
#include "ShaderTable.h"
#include "DXSampleHelper.h"
#include "GpuUploadBuffer.h"
#include "CompiledShaders\Raytracing.hlsl.h"

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

const wchar_t* CRaytracingObjectTest::c_raygenShaderName = L"MyRaygenShader";
const wchar_t* CRaytracingObjectTest::c_missShaderName = L"MyMissShader";
const wchar_t* CRaytracingObjectTest::c_hitGroupName = L"MyHitGroup";
const wchar_t* CRaytracingObjectTest::c_closestHitShaderName = L"MyClosestHitShader";

// Library subobject names
const wchar_t* CRaytracingObjectTest::c_globalRootSignatureName = L"MyGlobalRootSignature";
const wchar_t* CRaytracingObjectTest::c_localRootSignatureName =  L"MyLocalRootSignature";
const wchar_t* CRaytracingObjectTest::c_localRootSignatureAssociationName = L"MyLocalRootSignatureAssociation";
const wchar_t* CRaytracingObjectTest::c_shaderConfigName = L"MyShaderConfig";
const wchar_t* CRaytracingObjectTest::c_pipelineConfigName = L"MyPipelineConfig";

void CRaytracingObjectTest::Init()
{

	m_raytracingOutputResourceUAVDescriptorHeapIndex = UINT_MAX;//***
	auto frameIndex = D3D::Get()->FrameCount();

	// Setup camera.
	{
		// Initialize the view and projection inverse matrices.
		m_eye = { 0.0f, 2.0f, -5.0f, 1.0f };
		m_at = { 0.0f, 0.0f, 0.0f, 1.0f };
		XMVECTOR right = { 1.0f, 0.0f, 0.0f, 0.0f };

		XMVECTOR direction = XMVector4Normalize(m_at - m_eye);
		m_up = XMVector3Normalize(XMVector3Cross(direction, right));

		// Rotate camera around Y axis.
		XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(45.0f));
		m_eye = XMVector3Transform(m_eye, rotate);
		m_up = XMVector3Transform(m_up, rotate);

		UpdateCameraMatrices();
	}

	// Setup lights.
	{
		// Initialize the lighting parameters.
		XMFLOAT4 lightPosition;
		XMFLOAT4 lightAmbientColor;
		XMFLOAT4 lightDiffuseColor;

		lightPosition = XMFLOAT4(1.0f, 1.8f, -3.0f, 0.0f);
		m_sceneCB[frameIndex].lightPosition = XMLoadFloat4(&lightPosition);

		lightAmbientColor = XMFLOAT4(1.5f, 0.5f, 1.5f, 1.0f);
		m_sceneCB[frameIndex].lightAmbientColor = XMLoadFloat4(&lightAmbientColor);

		lightDiffuseColor = XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f);
		m_sceneCB[frameIndex].lightDiffuseColor = XMLoadFloat4(&lightDiffuseColor);
	}

	for (auto& sceneCB : m_sceneCB)
	{
		sceneCB = m_sceneCB[frameIndex];
	}
	InitRootSinagture();
	InitPipelineState();
	CreateDescriptorHeap();
	Mesh();
	InitAccelerationStructures();
	CreateConstantBuffers();
	InitShaderTables();
	CreateRaytracingOutputResource();

	CreateRaytracingOutputResource();
	UpdateCameraMatrices();
	m_matScale = XMMatrixIdentity();
	m_matRot = XMMatrixIdentity();
	m_matTrans = XMMatrixIdentity();
	m_matWorld = XMMatrixIdentity();

	SetTrasform(Vector3(0, 0, 0));

	m_dwInitRefCount++;

}


BOOL CRaytracingObjectTest::InitRootSinagture()
{
	// Workaround

	//auto device = m_deviceResources->GetDevice();

	// A unique global root signature is defined in hlsl library g_pRaytracing. For such scenario we can create 
	// compute root signature can directly from the library bytecode, using CreateRootSignature API. 
	ThrowIfFailed(D3D::GetDevice()->CreateRootSignature(1, g_pRaytracing, ARRAYSIZE(g_pRaytracing), IID_PPV_ARGS(&m_raytracingGlobalRootSignature)));


	// Global Root Signature
	// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
	// 글로벌 루트 서명
	// 이는 DispatchRays() 호출 중에 호출된 모든 레이트레이싱 셰이더에서 공유되는 루트 서명입니다.
	//{
	//	CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
	//	UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	//	CD3DX12_ROOT_PARAMETER rootParameters[2];
	//	rootParameters[0].InitAsDescriptorTable(1, &UAVDescriptor);
	//	rootParameters[1].InitAsShaderResourceView(0);
	//	CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
	//	SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, m_raytracingGlobalRootSignature);


	//}

	//// Local Root Signature
	//// This is a root signature that enables a shader to have unique arguments that come from shader tables.
	//// 로컬 루트 서명
	//// 이것은 셰이더가 셰이더 테이블에서 나오는 고유한 인수를 가질 수 있도록 하는 루트 서명입니다.
	//{
	//	CD3DX12_ROOT_PARAMETER rootParameters[1];
	//	rootParameters[0].InitAsConstants(SizeOfInUint32(m_rayGenCB), 0, 0);
	//	CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
	//	localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	//	SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, m_raytracingLocalRootSignature);
	//}





	return TRUE;
}



BOOL CRaytracingObjectTest::InitPipelineState()
{

	struct
	{
		std::byte* data;
		uint32_t size;
	} RayShader;

	//ReadDataFromFile(L"Raytracing.cso", &RayShader.data, &RayShader.size);

	// Create library and use the library subobject defined in the library in the RTPSO:
	// Subobjects need to be associated with DXIL shaders exports either by way of default or explicit associations.
	// Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
	// This simple sample utilizes default shader association except for local root signature subobject
	// which has an explicit association specified purely for demonstration purposes.
	//
	// Following subobjects are defined with the library itself. We can export and rename the subobjects from the 
	// DXIL library in a similar way we export shaders. 
	// 1 - Triangle hit group
	// 1 - Shader config
	// 2 - Local root signature and association
	// 1 - Global root signature
	// 1 - Pipeline config
	// 1 - Subobject to export association
	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


	// DXIL library
	// This contains the shaders and their entrypoints for the state object.
	// Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
	auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	//D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(RayShader.data, RayShader.size);
	D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));

	lib->SetDXILLibrary(&libdxil);
	// Define which shader exports to surface from the library.
	// If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
	// In this sample, this could be ommited for convenience since the sample uses all shaders in the library. 
	{
		lib->DefineExport(c_raygenShaderName);
		lib->DefineExport(c_closestHitShaderName);
		lib->DefineExport(c_missShaderName);
	}

	// Define which subobjects exports to use from the library.
	// If no exports are defined all subobjects are used. 
	{
		lib->DefineExport(c_globalRootSignatureName);
		lib->DefineExport(c_localRootSignatureName);
		lib->DefineExport(c_localRootSignatureAssociationName);
		lib->DefineExport(c_shaderConfigName);
		lib->DefineExport(c_pipelineConfigName);
		lib->DefineExport(c_hitGroupName);
	}

	ThrowIfFailed(D3D::GetDevice()->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_dxrStateObject)), L"Couldn't create DirectX Raytracing state object.\n");

	return TRUE;
}

void CRaytracingObjectTest::InitAccelerationStructures()
{
	ResourceManager* pResourceManager = D3D::Get()->GetResourceManager();
	auto device = D3D::GetDevice();
	auto commandList = pResourceManager->GetCommandList();
	auto commandQueue = pResourceManager->GetCommandQueue();
	auto commandAllocator = pResourceManager->GetCommandAllocator();

	//auto commandList = D3D::Get()->GetCurrentCommandList();
	//auto commandQueue = D3D::Get()->GetCommandQueue();
	//auto commandAllocator = D3D::Get()->GetCommandAllocator();


	// Reset the command list for the acceleration structure construction.
	commandList->Reset(commandAllocator, nullptr);

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.IndexBuffer = m_indexBuffer.resource->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(m_indexBuffer.resource->GetDesc().Width) / sizeof(Index);
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(m_vertexBuffer.resource->GetDesc().Width) / sizeof(Vertex);
	geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer.resource->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

	// Mark the geometry as opaque. 
	// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
	// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	// Get required sizes for an acceleration structure.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.Flags = buildFlags;
	bottomLevelInputs.NumDescs = 1;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = &geometryDesc;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.pGeometryDescs = nullptr;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

	if (!(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0)) { __debugbreak(); }



	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};

	device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
	if (!(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0)) { __debugbreak(); }

	ID3D12Resource* scratchResource;
	AllocateUAVBuffer(device, max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

	// Allocate resources for acceleration structures.
	// Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
	// Default heap is OK since the application doesn뭪 need CPU read/write access to them. 
	// The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
	// and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
	//  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
	//  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
	{
		D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		AllocateUAVBuffer(device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
		AllocateUAVBuffer(device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
	}

	ID3D12Resource* instanceDescs;
	{
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
		instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
		instanceDesc.InstanceMask = 1;
		instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
		AllocateUploadBuffer(device, &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");
	}

	// Bottom Level Acceleration Structure desc
	{
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
	}

	// Top Level Acceleration Structure desc
	{
		topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
	}

	auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
	{
		raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
		raytracingCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure));
		raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
	};

	BuildAccelerationStructure(commandList);

	//// Kick off acceleration structure construction.
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	// Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
	pResourceManager->Fence();
	pResourceManager->WaitForFenceValue();
	// 
	// Kick off acceleration structure construction.
	//D3D::Get()->ExecuteCommandList();

	//// Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
	//D3D::Get()->WaitForGpu();

}

void CRaytracingObjectTest::InitShaderTables()
{
	ResourceManager* pResourceManager = D3D::Get()->GetResourceManager();

	auto device = D3D::GetDevice();
	void* rayGenShaderIdentifier;
	void* missShaderIdentifier;
	void* hitGroupShaderIdentifier;

	auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
	{
		rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_raygenShaderName);
		missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_missShaderName);
		hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_hitGroupName);
	};
	// Get shader identifiers.
	UINT shaderIdentifierSize;
	{
		ID3D12StateObjectProperties* stateObjectProperties;
		ThrowIfFailed(m_dxrStateObject->QueryInterface(&stateObjectProperties));
		GetShaderIdentifiers(stateObjectProperties);
		shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}

	// Ray gen shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable rayGenShaderTable(device, numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
		rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize));
		m_rayGenShaderTable = rayGenShaderTable.GetResource();
	}

	// Miss shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(device, numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
		m_missShaderTable = missShaderTable.GetResource();
	}

	// Hit group shader table
	{
		struct RootArguments {
			CubeConstantBuffer cb;
		} rootArguments;
		rootArguments.cb = m_cubeCB;

		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);//LocalRootSignature
		ShaderTable hitGroupShaderTable(device, numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
		hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
		m_hitGroupShaderTable = hitGroupShaderTable.GetResource();
	}
}

void CRaytracingObjectTest::CreateRaytracingOutputResource()
{
	auto device = D3D::GetDevice();
	auto backbufferFormat = D3D::Get()->GetBackBufferFormat();

	// Create the output resource. The dimensions and format should match the swap-chain.
	auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(backbufferFormat, D3D::GetDesc().Width, D3D::GetDesc().Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_raytracingOutput)));
	//NAME_D3D12_OBJECT(m_raytracingOutput);

	D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;
	m_raytracingOutputResourceUAVDescriptorHeapIndex = AllocateDescriptor(&uavDescriptorHandle, m_raytracingOutputResourceUAVDescriptorHeapIndex);
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(m_raytracingOutput, nullptr, &UAVDesc, uavDescriptorHandle);
	m_raytracingOutputResourceUAVGpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), m_raytracingOutputResourceUAVDescriptorHeapIndex, m_descriptorSize);
}

void CRaytracingObjectTest::CopyRaytracingOutputToBackbuffer()
{
	auto commandList = D3D::Get()->GetCurrentCommandList();
	auto renderTarget = D3D::Get()->CurrentBackBuffer();

	D3D12_RESOURCE_BARRIER preCopyBarriers[2];
	preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	commandList->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

	commandList->CopyResource(renderTarget, m_raytracingOutput);

	D3D12_RESOURCE_BARRIER postCopyBarriers[2];
	postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(renderTarget, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);

}

void CRaytracingObjectTest::UpdateCameraMatrices()
{
	auto frameIndex = D3D::Get()->FrameCount();
	m_sceneCB[frameIndex].cameraPosition = Context::Get()->GetCamera()->GetPosition();
	float fovAngleY = 45.0f;
	XMMATRIX view = Context::Get()->View();
	XMMATRIX proj = Context::Get()->Projection();
	XMMATRIX viewProj = view * proj;

	//float aspectRatio = static_cast<float>(D3D::GetDesc().Width) / static_cast<float>(D3D::GetDesc().Height);
	//m_sceneCB[frameIndex].cameraPosition = m_eye;
	//float fovAngleY = 45.0f;
	//XMMATRIX view = XMMatrixLookAtLH(m_eye, m_at, m_up);
	//XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(fovAngleY), aspectRatio, 1.0f, 125.0f);
	//XMMATRIX viewProj = view * proj;


	m_sceneCB[frameIndex].projectionToWorld = XMMatrixInverse(nullptr, viewProj);
}

void CRaytracingObjectTest::CreateConstantBuffers()
{
	auto frameCount = 2;

	// Create the constant buffer memory and map the CPU and GPU addresses
	const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Allocate one constant buffer per frame, since it gets updated every frame.
	size_t cbSize = frameCount * sizeof(AlignedSceneConstantBuffer);
	const D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

	ThrowIfFailed(D3D::GetDevice()->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_perFrameConstants)));

	// Map the constant buffer and cache its heap pointers.
	// We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_perFrameConstants->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedConstantData)));


}

void CRaytracingObjectTest::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature* rootSig)
{
	ID3DBlob* pSignature = nullptr;
	ID3DBlob* pError = nullptr;
	if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError)))
	{
		__debugbreak();
	}

	if (FAILED(D3D::GetDevice()->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&rootSig))))
	{
		__debugbreak();
	}
	if (pSignature)
	{
		pSignature->Release();
		pSignature = nullptr;
	}
	if (pError)
	{
		pError->Release();
		pError = nullptr;
	}
}

UINT CRaytracingObjectTest::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse)
{
	auto descriptorHeapCpuBase = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	if (descriptorIndexToUse >= mSrvDescriptorHeap->GetDesc().NumDescriptors)
	{
		descriptorIndexToUse = m_descriptorsAllocated++;
	}
	*cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, m_descriptorSize);
	return descriptorIndexToUse;
}

UINT CRaytracingObjectTest::CreateBufferSRV(D3DBuffer* buffer, UINT numElements, UINT elementSize)
{
	auto device = D3D::GetDevice();

	// SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = numElements;
	if (elementSize == 0)
	{
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		srvDesc.Buffer.StructureByteStride = 0;
	}
	else
	{
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		srvDesc.Buffer.StructureByteStride = elementSize;
	}
	UINT descriptorIndex = AllocateDescriptor(&buffer->cpuDescriptorHandle);
	device->CreateShaderResourceView(buffer->resource, &srvDesc, buffer->cpuDescriptorHandle);
	buffer->gpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), descriptorIndex, m_descriptorSize);
	return descriptorIndex;
}

void CRaytracingObjectTest::CreateDescriptorHeap()
{
	auto device = D3D::GetDevice();

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	// Allocate a heap for 3 descriptors:
	// 2 - vertex and index buffer SRVs
	// 1 - raytracing output texture SRV
	descriptorHeapDesc.NumDescriptors = 3;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;
	device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap));
	//NAME_D3D12_OBJECT(mSrvDescriptorHeap);

	m_descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

}



void CRaytracingObjectTest::Mesh()
{
	ResourceManager* pResourceManager = D3D::Get()->GetResourceManager();
	auto device = D3D::GetDevice();

	MeshData meshData = GeometryGenerator::MakeBox(1.0f);
	//GeometryGenerator::MakeSphere(1.5f, 15, 13);

	std::vector<std::uint16_t> indices = meshData.GetIndices16();
	count = indices.size();

	//AllocateUploadBuffer(device, meshData.GetIndices16().data(), sizeof(UINT16) * meshData.GetIndices16().size(), &m_indexBuffer.resource);
	//AllocateUploadBuffer(device, meshData.vertices.data(), sizeof(meshData.vertices[0]) * meshData.vertices.size(), &m_vertexBuffer.resource);

	//// Vertex buffer is passed to the shader along with index buffer as a descriptor table.
	//// Vertex buffer descriptor must follow index buffer descriptor in the descriptor heap.
	//UINT descriptorIndexIB = CreateBufferSRV(&m_indexBuffer, meshData.GetIndices16().size() / 4, 0);
	//UINT descriptorIndexVB = CreateBufferSRV(&m_vertexBuffer, meshData.vertices.size(), sizeof(meshData.vertices[0]));
	//ThrowIfFalse(descriptorIndexVB == descriptorIndexIB + 1, L"Vertex Buffer descriptor index must follow that of Index Buffer descriptor index!");



	if (FAILED(pResourceManager->CreateStaticVertexBuffer(sizeof(Vertex), (DWORD)meshData.vertices.size(), &m_VertexBufferView, &m_vertexBuffer.resource, meshData.vertices.data())))
	{
		__debugbreak();
	}
	if (FAILED(pResourceManager->CreateIndexBuffer((DWORD)indices.size(), &m_IndexBufferView, &m_indexBuffer.resource, indices.data())))
	{
		__debugbreak();
	}
	UINT descriptorIndexIB = CreateBufferSRV(&m_indexBuffer, indices.size()/4, 0);
	UINT descriptorIndexVB = CreateBufferSRV(&m_vertexBuffer, meshData.vertices.size(), sizeof(meshData.vertices[0]));

	//for (MeshData& meshData : meshes)
	//{
	//	SMesh* newMesh = new SMesh();
	//	if (FAILED(pResourceManager->CreateStaticVertexBuffer(sizeof(Vertex), (DWORD)meshData.vertices.size(), &newMesh->vertexBufferView, &newMesh->vertexBuffer, meshData.vertices.data())))
	//	{
	//		__debugbreak();
	//	}
	//	std::vector<std::uint16_t> indices = meshData.GetIndices16();
	//	newMesh->m_indexCount = indices.size();

	//	if (FAILED(pResourceManager->CreateIndexBuffer((DWORD)indices.size(), &newMesh->indexBufferView, &newMesh->indexBuffer, indices.data())))
	//	{
	//		__debugbreak();
	//	}



	//	if (!meshData.albedoTextureFilename.empty()) {

	//		//cout << meshData.albedoTextureFilename << endl;
	//		wstring message_w;
	//		newMesh->texture = new CTexture(message_w.assign(meshData.albedoTextureFilename.begin(), meshData.albedoTextureFilename.end()));

	//		//delete t;
	//	}
	//	newMesh->vertexConstantBuffer = m_vertexConstantBuffer;
	//	newMesh->pixelConstantBuffer = m_pixelConstantBuffer;

	//	m_meshes.push_back(newMesh);
	//	m_indexBuffer.resource = newMesh->indexBuffer;
	//	m_vertexBuffer.resource = newMesh->vertexBuffer;
	//	UINT descriptorIndexIB = CreateBufferSRV(&m_indexBuffer, sizeof(indices) / 4, 0);
	//	UINT descriptorIndexVB = CreateBufferSRV(&m_vertexBuffer, meshData.vertices.size(), sizeof(meshData.vertices[0]));

	//}



}


void CRaytracingObjectTest::TextureMaterial(wstring wchFileName)
{
	HRESULT hr;

	texture = new CTexture(wchFileName);
}

void CRaytracingObjectTest::Update()
{
	m_timer.Tick();
	//CalculateFrameStats();
	float elapsedTime = static_cast<float>(m_timer.GetElapsedSeconds());
	auto frameIndex = D3D::Get()->FrameCount();
	auto prevFrameIndex = frameIndex == 0 ? 3 - 1 : frameIndex - 1;
	// Rotate the camera around Y axis.
	{
		//float secondsToRotateAround = 24.0f;
		//float angleToRotateBy = 360.0f * (elapsedTime / secondsToRotateAround);
		//XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angleToRotateBy));
		//m_eye = XMVector3Transform(m_eye, rotate);
		//m_up = XMVector3Transform(m_up, rotate);
		//m_at = XMVector3Transform(m_at, rotate);
		UpdateCameraMatrices();
	}
	// Rotate the second light around Y axis.
	{
		float secondsToRotateAround = 8.0f;
		float angleToRotateBy = -360.0f * (elapsedTime / secondsToRotateAround);
		XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angleToRotateBy));
		const XMVECTOR& prevLightPosition = m_sceneCB[prevFrameIndex].lightPosition;
		m_sceneCB[frameIndex].lightPosition = XMVector3Transform(prevLightPosition, rotate);
	}
	//m_timer.Tick();
	////CalculateFrameStats();
	//float elapsedTime = static_cast<float>(m_timer.GetElapsedSeconds());
	//auto frameIndex = m_deviceResources->FrameCount();
	//auto prevFrameIndex = frameIndex == 0 ? 3 - 1 : frameIndex - 1;

	//// Rotate the camera around Y axis.
	//{
	//	float secondsToRotateAround = 24.0f;
	//	float angleToRotateBy = 360.0f * (elapsedTime / secondsToRotateAround);
	//	XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angleToRotateBy));
	//	m_eye = XMVector3Transform(m_eye, rotate);
	//	m_up = XMVector3Transform(m_up, rotate);
	//	m_at = XMVector3Transform(m_at, rotate);
	//	UpdateCameraMatrices();
	//}

	//// Rotate the second light around Y axis.
	//{
	//	float secondsToRotateAround = 8.0f;
	//	float angleToRotateBy = -360.0f * (elapsedTime / secondsToRotateAround);
	//	XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angleToRotateBy));
	//	const XMVECTOR& prevLightPosition = m_sceneCB[prevFrameIndex].lightPosition;
	//	m_sceneCB[frameIndex].lightPosition = XMVector3Transform(prevLightPosition, rotate);
	//}

	//m_BasicVertexConstantBufferData.model = XMMatrixTranspose(m_matWorld);
	////m_BasicVertexConstantBufferData.model.Transpose();

	//m_BasicVertexConstantBufferData.invTranspose =
	//	m_BasicVertexConstantBufferData.model;
	//m_BasicVertexConstantBufferData.invTranspose.Translation(Vector3(0.0f));
	//m_BasicVertexConstantBufferData.invTranspose =
	//	m_BasicVertexConstantBufferData.invTranspose.Transpose().Invert();

	//// 시점 변환
	//m_BasicVertexConstantBufferData.view = XMMatrixTranspose(Context::Get()->View());
	////Matrix::CreateRotationY(0.0f)*
	////	Matrix::CreateTranslation(0.0f, 0.0f, 2.0f);


	//m_BasicPixelConstantBufferData.eyeWorld = Vector3::Transform(
	//	Vector3(0.0f), m_BasicVertexConstantBufferData.view.Invert());

	////m_BasicVertexConstantBufferData.view =
	////	m_BasicVertexConstantBufferData.view.Transpose();

	//// 프로젝션
	//m_BasicVertexConstantBufferData.projection = XMMatrixTranspose(Context::Get()->Projection());

	//m_BasicPixelConstantBufferData.material.diffuse =
	//	Vector3(0.8f);
	//m_BasicPixelConstantBufferData.material.specular =
	//	Vector3(1.0f);

	//// 여러 개 조명 사용 예시
	//for (int i = 0; i < 3; i++) {
	//	// 다른 조명 끄기
	//	if (i != 0) {
	//		//m_BasicPixelConstantBufferData.lights[i].Strength *= 0.0f;
	//	}
	//	else {
	//		m_BasicPixelConstantBufferData.lights[i] = m_lightFromGUI;
	//	}
	//}

	//memcpy(m_vertexDataBegin + sizeof(BasicVertexConstantBuffer) * D3D::Get()->FrameCount(), &m_BasicVertexConstantBufferData, sizeof(m_BasicVertexConstantBufferData));
	//memcpy(m_pixelDataBegin + sizeof(BasicPixelConstantBuffer) * D3D::Get()->FrameCount(), &m_BasicPixelConstantBufferData, sizeof(m_BasicPixelConstantBufferData));

}

void CRaytracingObjectTest::Render()
{
	//D3D::Get()->GetRenderer()->RenderObjectPush(this);

#ifdef USE_MULTI_THREAD
#else
	//ID3D12GraphicsCommandList* pCommandList = D3D::Get()->GetCommandListPool(0)->GetCurrentCommandList();
	//ProcessRender(pCommandList,0);
#endif

	ID3D12GraphicsCommandList6* ppCommandList = D3D::Get()->GetCurrentCommandList();
	ProcessRender(ppCommandList, 0);

	//if (!m_ppRenderQueue[m_dwCurThreadIndex]->Add(&item))
//	__debugbreak();

//m_dwCurThreadIndex++;
//m_dwCurThreadIndex = m_dwCurThreadIndex % m_dwRenderThreadCount;

}

void CRaytracingObjectTest::ProcessRender(ID3D12GraphicsCommandList* pCommandList, DWORD ThreadIndex)
{
	//ID3D12GraphicsCommandList* pCommandList = D3D::Get()->GetCommandListPool(ThreadIndex)->GetCurrentCommandList();
	//UINT srvDescriptorSize = D3D::Get()->INL_GetSrvDescriptorSize();
	//CDescriptorPool* pDescriptorPool = D3D::Get()->INL_GetDescriptorPool(ThreadIndex);
	//ID3D12DescriptorHeap* pDescriptorHeap = pDescriptorPool->INL_GetDescriptorHeap();
	//CSimpleConstantBufferPool* pConstantBufferPool = D3D::Get()->GetConstantBufferPool(CONSTANT_BUFFER_TYPE_DEFAULT, ThreadIndex);
	//ID3D12DescriptorHeap* pDescriptorHeap = D3D::Get()->INL_GetSingleDescriptorAllocator()->GetDescriptorHeap();

	//D3D::Get()->SetRenderTarget(NULL, NULL, ThreadIndex);


	// set RootSignature



	auto commandList = D3D::Get()->GetCurrentCommandList();
	auto frameIndex = D3D::Get()->FrameCount();

	auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
	{
		// Since each shader table has only one shader record, the stride is same as the size.
		dispatchDesc->HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
		dispatchDesc->HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
		dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
		dispatchDesc->MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
		dispatchDesc->MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
		dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
		dispatchDesc->RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
		dispatchDesc->RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->GetDesc().Width;
		dispatchDesc->Width = D3D::GetDesc().Width;
		dispatchDesc->Height = D3D::GetDesc().Height;
		dispatchDesc->Depth = 1;
		commandList->SetPipelineState1(stateObject);
		commandList->DispatchRays(dispatchDesc);
	};

	//auto SetCommonPipelineState = [&](auto* descriptorSetCommandList)
	//{
	//	descriptorSetCommandList->SetDescriptorHeaps(1, &mSrvDescriptorHeap);
	//	// Set index and successive vertex buffer decriptor tables
	//	commandList->SetComputeRootDescriptorTable(3, m_indexBuffer.gpuDescriptorHandle);
	//	commandList->SetComputeRootDescriptorTable(0, m_raytracingOutputResourceUAVGpuDescriptor);
	//};

	commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature);

	// Copy the updated scene constant buffer to GPU.
	memcpy(&m_mappedConstantData[frameIndex].constants, &m_sceneCB[frameIndex], sizeof(m_sceneCB[frameIndex]));
	auto cbGpuAddress = m_perFrameConstants->GetGPUVirtualAddress() + frameIndex * sizeof(m_mappedConstantData[0]);
	commandList->SetComputeRootConstantBufferView(2, cbGpuAddress);//콘스탄트 버퍼

	// Bind the heaps, acceleration structure and dispatch rays.
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	{
		//SetCommonPipelineState(commandList);

		commandList->SetDescriptorHeaps(1, &mSrvDescriptorHeap);
		// Set index and successive vertex buffer decriptor tables
		commandList->SetComputeRootDescriptorTable(3, m_indexBuffer.gpuDescriptorHandle);// index/vertexbuffer
		commandList->SetComputeRootDescriptorTable(0, m_raytracingOutputResourceUAVGpuDescriptor);//RenderTarget

		commandList->SetComputeRootShaderResourceView(1, m_topLevelAccelerationStructure->GetGPUVirtualAddress());//가속구조체
		DispatchRays(commandList, m_dxrStateObject, &dispatchDesc);
	}

	CopyRaytracingOutputToBackbuffer();
}

void CRaytracingObjectTest::Destroy()
{
	Cleanup();
}



void CRaytracingObjectTest::SetTrasform(Vector3 vec)
{
	m_matTrans = XMMatrixTranslation(vec.x,vec.y,vec.z);


	m_matWorld = XMMatrixMultiply(m_matScale, m_matRot);
	m_matWorld = XMMatrixMultiply(m_matWorld, m_matTrans);
}



void CRaytracingObjectTest::Cleanup()
{
	for (auto mesh : m_meshes)
	{
		mesh->indexBuffer->Release();
		mesh->indexBuffer = nullptr;

		mesh->vertexBuffer->Release();
		mesh->vertexBuffer = nullptr;

		delete mesh->texture;
		mesh->texture = nullptr;
		//mesh->vertexConstantBuffer->Release();
		//mesh->vertexConstantBuffer = nullptr;

		//mesh->pixelConstantBuffer->Release();
		//mesh->pixelConstantBuffer = nullptr;

		//mesh->texture->Release();
		//mesh->texture = nullptr;

	}


	if (!m_dwInitRefCount)
		return;

	DWORD ref_count = --m_dwInitRefCount;
	if (!ref_count)
	{
		if (m_pRootSignature)
		{
			m_pRootSignature->Release();
			m_pRootSignature = nullptr;
		}
		if (m_pPipelineState)
		{
			m_pPipelineState->Release();
			m_pPipelineState = nullptr;
		}
	}
	if (pTexResource)
	{
		pTexResource->Release();
		pTexResource = nullptr;
	}
	if (textureUploadHeap)
	{
		textureUploadHeap->Release();
		textureUploadHeap = nullptr;
	}

	if (srv.ptr)
	{
		D3D::Get()->INL_GetSingleDescriptorAllocator()->FreeDescriptorHandle(srv);
		srv = {};
	}
	if (texture)
	{
		delete texture;
		texture = nullptr;
	}

}

