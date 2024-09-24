#include "pch.h"
#include "D3D.h"
#include "ResourceManager.h"
#include "DescriptorAllocator.h"
#include "Context.h"
#include "GeometryGenerator.h"
#include <DDSTextureLoader.h>
#include <fp16.h>
#include <assert.h>
#include "MeshletObjectTest.h"
#include "Texture.h"

void MeshletObjectTest::Init()
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


BOOL MeshletObjectTest::InitConstantBuffer()
{	
	
	const UINT64 constantBufferSize = sizeof(SceneConstantBuffer) * 3;

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



BOOL MeshletObjectTest::InitRootSinagtureAndPipelineState()
{
	ID3DBlob* pVertexShader = nullptr;
	ID3DBlob* pPixelShader = nullptr;


	struct
	{
		std::byte* data;
		uint32_t size;
	} meshShader, pixelShader;

	ReadDataFromFile(L"TestMS.cso", &meshShader.data, &meshShader.size);
	ReadDataFromFile(L"MeshletPS.cso", &pixelShader.data, &pixelShader.size);
	ThrowIfFailed(D3D::GetDevice()->CreateRootSignature(0, meshShader.data, meshShader.size, IID_PPV_ARGS(&m_pRootSignature)));

	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSignature;
	psoDesc.MS = { meshShader.data, meshShader.size };
	psoDesc.PS = { pixelShader.data, pixelShader.size };
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc = DefaultSampleDesc();
	auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
	streamDesc.pPipelineStateSubobjectStream = &psoStream;
	streamDesc.SizeInBytes = sizeof(psoStream);

	ThrowIfFailed(D3D::GetDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pPipelineState)));

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	return TRUE;

}

HRESULT MeshletObjectTest::ReadDataFromFile(LPCWSTR filename, std::byte** data, UINT* size)
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

void MeshletObjectTest::Mesh()
{
	ResourceManager* pResourceManager = D3D::Get()->GetResourceManager();

	vector<MeshData> meshes = GeometryGenerator::ReadFromFile("C:/Graphic/f3d-data/zelda-breath-of-the-wild/source/zeldaPosed001/", "zeldaPosed001.fbx");

	for (MeshData& meshData : meshes)
	{
		SMesh* newMesh = new SMesh();

		if (FAILED(pResourceManager->CreateStaticVertexBuffer(sizeof(Vertex), (DWORD)meshData.vertices.size(), &newMesh->vertexBufferView, &newMesh->vertexBuffer, meshData.vertices.data())))
		{
			__debugbreak();
		}
		std::vector<std::uint16_t> indices = meshData.GetIndices16();
		newMesh->m_indexCount = indices.size();

		if (FAILED(pResourceManager->CreateIndexBuffer((DWORD)indices.size(), &newMesh->indexBufferView, &newMesh->indexBuffer, indices.data())))
		{
			__debugbreak();
		}



		if (!meshData.albedoTextureFilename.empty()) {

			wstring message_w;
			newMesh->texture = new CTexture(message_w.assign(meshData.albedoTextureFilename.begin(), meshData.albedoTextureFilename.end()));

		}
		size_t nFaces = indices.size() / 3;
		size_t nVerts = meshData.vertices.size();

		auto pos = std::make_unique<XMFLOAT3[]>(nVerts);
		for (size_t j = 0; j < nVerts; ++j)
			pos[j] = meshData.vertices[j].position;

		auto adj = std::make_unique<uint32_t[]>(indices.size());
		ThrowIfFailed(GenerateAdjacencyAndPointReps(
			indices.data(), nFaces,
			pos.get(), nVerts, 0, nullptr, adj.get()));

		std::vector<std::pair<size_t, size_t>> meshletSubsets;
		vector<DirectX::Meshlet> meshlets;
		vector<uint8_t> uniqueVertexIB;
		vector<DirectX::MeshletTriangle> primitiveIndices;
		ThrowIfFailed(DirectX::ComputeMeshlets(indices.data(), nFaces,
			pos.get(), nVerts,
			nullptr,
			meshlets, uniqueVertexIB, primitiveIndices
		));

		ThrowIfFailed(pResourceManager->CreateResourceBuffer(sizeof(DirectX::Meshlet), (DWORD)meshlets.size(), &newMesh->meshletBuffer, meshlets.data()));
		ThrowIfFailed(pResourceManager->CreateResourceBuffer(sizeof(uint8_t), (DWORD)uniqueVertexIB.size(), &newMesh->uniqueVertexIBBuffer, uniqueVertexIB.data()));
		ThrowIfFailed(pResourceManager->CreateResourceBuffer(sizeof(DirectX::MeshletTriangle), (DWORD)primitiveIndices.size(), &newMesh->primitiveIndicesBuffer, primitiveIndices.data()));

		newMesh->MeshletCount = meshlets.size();

		m_meshes.push_back(newMesh);
	}

	int n = 60;
	m_objects.resize(n * n);

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
			//XMStoreFloat4x4(&obj.World, m_matWorld);
			auto& obj = m_objects[index];


			obj.Model = m_meshes;
			obj.World = XMFLOAT4X4(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				x + j * dx, 0, y + i * dy, 1.0f);
			const CD3DX12_HEAP_PROPERTIES instanceBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
			const CD3DX12_RESOURCE_DESC instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Instance) * 3);

			// Create the per-object instance data buffer
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


void MeshletObjectTest::TextureMaterial(wstring wchFileName)
{
	HRESULT hr;

	texture = new CTexture(wchFileName);
}

void MeshletObjectTest::Update()
{
	XMMATRIX view = XMLoadFloat4x4(&Context::Get()->View());
	XMMATRIX proj = XMLoadFloat4x4(&Context::Get()->Projection());

	XMStoreFloat4x4(&m_constantBufferData.World, XMMatrixTranspose(m_matWorld));
	XMStoreFloat4x4(&m_constantBufferData.WorldView, XMMatrixTranspose(m_matWorld * view));
	XMStoreFloat4x4(&m_constantBufferData.WorldViewProj, XMMatrixTranspose(m_matWorld * view * proj));
	m_constantBufferData.DrawMeshlets = true;

	memcpy(m_cbvDataBegin + sizeof(SceneConstantBuffer) * D3D::Get()->FrameCount(), &m_constantBufferData, sizeof(m_constantBufferData));

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

void MeshletObjectTest::Render()
{

	ID3D12GraphicsCommandList6* pCommandList = D3D::Get()->GetCurrentCommandList();
	pCommandList->SetPipelineState(m_pPipelineState);
	//pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	pCommandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * D3D::Get()->FrameCount());

	for (auto obj : m_objects)
	{
		pCommandList->SetGraphicsRootConstantBufferView(6, obj.InstanceResource->GetGPUVirtualAddress() + sizeof(Instance) * D3D::Get()->FrameCount());

		for (auto mesh : obj.Model)
		{
			pCommandList->SetGraphicsRootShaderResourceView(2, mesh->vertexBuffer->GetGPUVirtualAddress());
			pCommandList->SetGraphicsRootShaderResourceView(3, mesh->meshletBuffer->GetGPUVirtualAddress());
			pCommandList->SetGraphicsRootShaderResourceView(4, mesh->uniqueVertexIBBuffer->GetGPUVirtualAddress());
			pCommandList->SetGraphicsRootShaderResourceView(5, mesh->primitiveIndicesBuffer->GetGPUVirtualAddress());

			//ppCommandList->SetGraphicsRoot32BitConstant(1, 0, 1);
			pCommandList->DispatchMesh(mesh->MeshletCount, 1, 1);

		}

	}

}

void MeshletObjectTest::Destroy()
{
	Cleanup();
}



void MeshletObjectTest::SetTrasform(Vector3 vec)
{
	m_matTrans = XMMatrixTranslation(vec.x,vec.y,vec.z);


	m_matWorld = XMMatrixMultiply(m_matScale, m_matRot);
	m_matWorld = XMMatrixMultiply(m_matWorld, m_matTrans);
}



void MeshletObjectTest::Cleanup()
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
	if (texture)
	{
		delete texture;
		texture = nullptr;
	}
	m_meshes.clear();
}

