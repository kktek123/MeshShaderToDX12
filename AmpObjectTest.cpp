#include "pch.h"
#include "D3D.h"
#include "ResourceManager.h"
#include "DescriptorAllocator.h"
#include "Context.h"
#include "GeometryGenerator.h"
#include <DDSTextureLoader.h>
#include <fp16.h>
#include <assert.h>
#include "Texture.h"
#include "AmpObjectTest.h"
#include "D3D12MeshletGenerator.h"

void AmpObjectTest::Init()
{
	InitConstantBuffer();
	InitRootSinagtureAndPipelineState();
	Mesh();

	m_matScale = XMMatrixIdentity();
	m_matRot = XMMatrixIdentity();
	m_matTrans = XMMatrixIdentity();
	m_matWorld = XMMatrixIdentity();

	SetTrasform(Vector3(0, 0, 0));

	m_dwInitRefCount++;

}


BOOL AmpObjectTest::InitConstantBuffer()
{	
	
	const UINT64 constantBufferSize = sizeof(Constants) * 3;

	const CD3DX12_HEAP_PROPERTIES constantBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	const CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

	ThrowIfFailed(D3D::GetDevice()->CreateCommittedResource(
		&constantBufferHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&constantBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantBuffer)));

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constantBufferSize;

	// Map and initialize the constant buffer. We don't unmap this until the
	// app closes. Keeping things mapped for the lifetime of the resource is okay.
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvDataBegin)));



	return TRUE;
}



BOOL AmpObjectTest::InitRootSinagtureAndPipelineState()
{
	struct
	{
		std::byte* data;
		uint32_t size;
	} ampShader, meshShader, pixelShader;

	ReadDataFromFile(L"MeshletAS.cso", &ampShader.data, &ampShader.size);
	ReadDataFromFile(L"TestMS2.cso", &meshShader.data, &meshShader.size);
	ReadDataFromFile(L"TestPS.cso", &pixelShader.data, &pixelShader.size);
	ThrowIfFailed(D3D::GetDevice()->CreateRootSignature(0, ampShader.data, ampShader.size, IID_PPV_ARGS(&m_pRootSignature)));

	// Disable culling so we can see the backside of geometry through the culled mesh.
	CD3DX12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterDesc.CullMode = D3D12_CULL_MODE_BACK;


	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSignature;
	psoDesc.AS = { ampShader.data, ampShader.size };
	psoDesc.MS = { meshShader.data, meshShader.size };
	psoDesc.PS = { pixelShader.data, pixelShader.size };
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.RasterizerState = rasterDesc;    // CW front; cull back
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc = DefaultSampleDesc();
	auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
	streamDesc.pPipelineStateSubobjectStream = &psoStream;
	streamDesc.SizeInBytes = sizeof(psoStream);

	ThrowIfFailed(D3D::GetDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pPipelineState)));

	return TRUE;

}

HRESULT AmpObjectTest::ReadDataFromFile(LPCWSTR filename, std::byte** data, UINT* size)
{
	using namespace Microsoft::WRL;

	CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
	extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extendedParams.lpSecurityAttributes = nullptr;
	extendedParams.hTemplateFile = nullptr;

	Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
	if (file.Get() == INVALID_HANDLE_VALUE)
	{
		throw std::exception();
	}

	FILE_STANDARD_INFO fileInfo = {};
	if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
	{
		throw std::exception();
	}

	if (fileInfo.EndOfFile.HighPart != 0)
	{
		throw std::exception();
	}

	*data = reinterpret_cast<std::byte*>(malloc(fileInfo.EndOfFile.LowPart));
	*size = fileInfo.EndOfFile.LowPart;

	if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
	{
		throw std::exception();
	}

	return S_OK;
}

void AmpObjectTest::Mesh()
{
	ResourceManager* pResourceManager = D3D::Get()->GetResourceManager();


	vector<MeshData> meshes = GeometryGenerator::ReadFromFile("C:/Graphic/f3d-data/zelda-breath-of-the-wild/source/zeldaPosed001/", "zeldaPosed001.fbx");

	for (MeshData& meshData : meshes)
	{
		SMesh* newMesh = new SMesh();


		ThrowIfFailed(pResourceManager->CreateStaticVertexBuffer(sizeof(Vertex), (DWORD)meshData.vertices.size(), &newMesh->vertexBufferView, &newMesh->vertexBuffer, meshData.vertices.data()));
		std::vector<std::uint16_t> indices = meshData.GetIndices16();
		newMesh->m_indexCount = indices.size();
		ThrowIfFailed(pResourceManager->CreateIndexBuffer((DWORD)indices.size(), &newMesh->indexBufferView, &newMesh->indexBuffer, indices.data()));

		uint32_t nFaces = indices.size() / 3;
		size_t nVerts = meshData.vertices.size();

		auto pos = std::make_unique<XMFLOAT3[]>(nVerts);
		for (size_t j = 0; j < nVerts; ++j)
			pos[j] = meshData.vertices[j].position;

		vector<Meshlet> meshlets;
		vector<uint8_t> uniqueVertexIB;
		vector<PackedTriangle> primitiveIndices;
		vector<Subset> meshletSubsets;

		ThrowIfFailed(ComputeMeshlets(
			64, 126,
			indices.data(), indices.size(),
			pos.get(), nVerts,
			meshletSubsets,
			meshlets, uniqueVertexIB, primitiveIndices));

		ThrowIfFailed(pResourceManager->CreateResourceBuffer(sizeof(Meshlet), (DWORD)meshlets.size(), &newMesh->meshletBuffer, meshlets.data()));
		ThrowIfFailed(pResourceManager->CreateResourceBuffer(sizeof(uint8_t), (DWORD)uniqueVertexIB.size(), &newMesh->uniqueVertexIBBuffer, uniqueVertexIB.data()));
		ThrowIfFailed(pResourceManager->CreateResourceBuffer(sizeof(PackedTriangle), (DWORD)primitiveIndices.size(), &newMesh->primitiveIndicesBuffer, primitiveIndices.data()));

		MeshInfo info = {};
		info.IndexSize = indices.size();
		info.MeshletCount = meshlets.size();
		info.LastMeshletVertCount = meshlets.back().VertCount;
		info.LastMeshletPrimCount = meshlets.back().PrimCount;

		ThrowIfFailed(pResourceManager->CreateResourceBuffer(sizeof(MeshInfo), (DWORD)1, &newMesh->meshInfoBuffer, &info));

		vector<CullData> cullData;
		cullData.resize(meshlets.size());

		auto uniqueVertexIndices = reinterpret_cast<const uint16_t*>(uniqueVertexIB.data());

		ThrowIfFailed(ComputeCullData(
			pos.get(), nVerts,
			meshlets.data(), meshlets.size(),
			uniqueVertexIndices,
			primitiveIndices.data(),
			0x1,
			cullData.data()));

		ThrowIfFailed(pResourceManager->CreateResourceBuffer(sizeof(CullData), (DWORD)cullData.size(), &newMesh->CullDataBuffer, cullData.data()));

		newMesh->MeshletCount = meshlets.size();

		m_meshes.push_back(newMesh);

	}

	int n = 60;
	m_objects.resize(n*n);

	float width = 20.0f;
	float height = 20.0f;
	float depth = 20.0f;

	float x = -0.5f * width;
	float y = -0.5f * height;
	float z = -0.5f * depth;
	float dx = width / (n - 1);
	float dy = height / (n - 1);
	float dz = depth / (n - 1);


	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			int index = i * n + j;
			auto &obj = m_objects[index];


			obj.Model = m_meshes;
			obj.World = XMFLOAT4X4(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				x + j * dx, 0, y + i * dy, 1.0f);
			const CD3DX12_HEAP_PROPERTIES instanceBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Instance) * 3);
			obj.Flags |= CULL_FLAG;
			obj.Flags |= MESHLET_FLAG;

			ThrowIfFailed(D3D::GetDevice()->CreateCommittedResource(
				&instanceBufferHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&instanceBufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&obj.InstanceResource)
			));

			ThrowIfFailed(obj.InstanceResource->Map(0, nullptr, &obj.InstanceData));
		}
	}



}


void AmpObjectTest::Update()
{
	//m_matWorld = XMMATRIX(g_XMIdentityR0, g_XMIdentityR1, g_XMIdentityR2, g_XMIdentityR3);

	XMMATRIX view = XMLoadFloat4x4(&Context::Get()->View());
	XMMATRIX proj = XMLoadFloat4x4(&Context::Get()->Projection());
	XMMATRIX viewInv = XMMatrixInverse(nullptr, view);
	XMVECTOR scale, rot, viewPosition;
	XMMatrixDecompose(&scale, &rot, &viewPosition, viewInv);


	XMStoreFloat4x4(&m_constantBufferData.World, XMMatrixTranspose(m_matWorld));
	XMStoreFloat4x4(&m_constantBufferData.WorldView, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_constantBufferData.WorldViewProj, XMMatrixTranspose(view * proj));
	m_constantBufferData.DrawMeshlets = true;
	XMStoreFloat3(&m_constantBufferData.CullViewPosition, viewPosition);

	XMMATRIX vp = XMMatrixTranspose(view * proj);
	XMVECTOR planes[6] =
	{
		XMPlaneNormalize(vp.r[3] + vp.r[0]), // Left
		XMPlaneNormalize(vp.r[3] - vp.r[0]), // Right
		XMPlaneNormalize(vp.r[3] + vp.r[1]), // Bottom
		XMPlaneNormalize(vp.r[3] - vp.r[1]), // Top
		XMPlaneNormalize(vp.r[2]),           // Near
		XMPlaneNormalize(vp.r[3] - vp.r[2]), // Far
	};


	for (uint32_t i = 0; i < _countof(planes); ++i)
	{
		XMStoreFloat4(&m_constantBufferData.Planes[i], planes[i]);
	}

	m_constantBufferData.DrawMeshlets = true;

	memcpy(m_cbvDataBegin + sizeof(Constants) * D3D::Get()->FrameCount(), &m_constantBufferData, sizeof(m_constantBufferData));


	for (auto& obj : m_objects)
	{
		XMMATRIX world = XMLoadFloat4x4(&obj.World);

		XMVECTOR scale, rot, pos;
		XMMatrixDecompose(&scale, &rot, &pos, world);

		auto& instance = *(reinterpret_cast<Instance*>(obj.InstanceData) + D3D::Get()->FrameCount());
		XMStoreFloat4x4(&instance.World, XMMatrixTranspose(world));
		XMStoreFloat4x4(&instance.WorldInvTrans, XMMatrixTranspose(XMMatrixInverse(nullptr, XMMatrixTranspose(world))));
		instance.Scale = XMVectorGetX(scale);
		instance.Flags = obj.Flags;
	}

}

void AmpObjectTest::Render()
{
	ID3D12GraphicsCommandList6* ppCommandList = D3D::Get()->GetCurrentCommandList();

	ppCommandList->SetPipelineState(m_pPipelineState);
	ppCommandList->SetGraphicsRootSignature(m_pRootSignature);

	ppCommandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + sizeof(Constants) * D3D::Get()->FrameCount());



	for (auto obj : m_objects)
	{
		ppCommandList->SetGraphicsRootConstantBufferView(2, obj.InstanceResource->GetGPUVirtualAddress() + sizeof(Instance) * D3D::Get()->FrameCount());

		for (auto mesh : obj.Model)
		{
			ppCommandList->SetGraphicsRootConstantBufferView(1, mesh->meshInfoBuffer->GetGPUVirtualAddress());
			ppCommandList->SetGraphicsRootShaderResourceView(3, mesh->vertexBuffer->GetGPUVirtualAddress());
			ppCommandList->SetGraphicsRootShaderResourceView(4, mesh->meshletBuffer->GetGPUVirtualAddress());
			ppCommandList->SetGraphicsRootShaderResourceView(5, mesh->uniqueVertexIBBuffer->GetGPUVirtualAddress());
			ppCommandList->SetGraphicsRootShaderResourceView(6, mesh->primitiveIndicesBuffer->GetGPUVirtualAddress());
			ppCommandList->SetGraphicsRootShaderResourceView(7, mesh->CullDataBuffer->GetGPUVirtualAddress());
			ppCommandList->DispatchMesh(DivRoundUp(mesh->MeshletCount, AS_GROUP_SIZE), 1, 1);

		}

	}
}

void AmpObjectTest::Destroy()
{
	Cleanup();
}



void AmpObjectTest::SetTrasform(Vector3 vec)
{
	m_matTrans = XMMatrixTranslation(vec.x,vec.y,vec.z);


	m_matWorld = XMMatrixMultiply(m_matScale, m_matRot);
	m_matWorld = XMMatrixMultiply(m_matWorld, m_matTrans);
}



void AmpObjectTest::Cleanup()
{
	for (auto mesh : m_meshes)
	{
		mesh->indexBuffer->Release();
		mesh->indexBuffer = nullptr;

		mesh->vertexBuffer->Release();
		mesh->vertexBuffer = nullptr;

		mesh->meshletBuffer->Release();
		mesh->meshletBuffer = nullptr;
		mesh->uniqueVertexIBBuffer->Release();
		mesh->uniqueVertexIBBuffer = nullptr;
		mesh->primitiveIndicesBuffer->Release();
		mesh->primitiveIndicesBuffer = nullptr;
		mesh->meshInfoBuffer->Release();
		mesh->meshInfoBuffer = nullptr;
		mesh->CullDataBuffer->Release();
		mesh->CullDataBuffer = nullptr;

		delete mesh->texture;
		mesh->texture = nullptr;

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

	if (m_constantBuffer)
	{
		m_constantBuffer->Release();
		m_constantBuffer = nullptr;
	}
	for (auto obj : m_objects)
	{
		obj.InstanceResource->Release();
		obj.InstanceResource = nullptr;
	}
	m_meshes.clear();
	m_objects.clear();
}

