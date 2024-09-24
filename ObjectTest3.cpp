#include "pch.h"
#include "D3D.h"
#include "ResourceManager.h"
#include "DescriptorAllocator.h"
#include "Context.h"
#include "GeometryGenerator.h"
#include <DDSTextureLoader.h>
#include <fp16.h>
#include <assert.h>
#include "ObjectTest3.h"
#include "Texture.h"

void CObjectTest3::Init()
{
	InitConstantBuffer();
	InitRootSinagture();
	InitPipelineState();
	Mesh();

	m_matScale = XMMatrixIdentity();
	m_matRot = XMMatrixIdentity();
	m_matTrans = XMMatrixIdentity();
	m_matWorld = XMMatrixIdentity();

	SetTrasform(Vector3(0, 0, 0));

	m_dwInitRefCount++;

}


BOOL CObjectTest3::InitRootSinagture()
{
	CD3DX12_DESCRIPTOR_RANGE ranges[1] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	// b0 : Constant Buffer View

	CD3DX12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(0);
	rootParameters[2].InitAsConstantBufferView(1);
	rootParameters[3].InitAsConstantBufferView(2);

	// default sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0.0f;
	sampler.MaxAnisotropy = 16;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	sampler.MinLOD = -FLT_MAX;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// Allow input layout and deny uneccessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;


	// Create an empty root signature.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* pSignature = nullptr;
	ID3DBlob* pError = nullptr;

	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError)))
	{
		__debugbreak();
	}

	if (FAILED(D3D::GetDevice()->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))))
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

	return TRUE;
}



BOOL CObjectTest3::InitPipelineState()
{
	ID3DBlob* pVertexShader = nullptr;
	ID3DBlob* pPixelShader = nullptr;


#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	if (FAILED(D3DCompileFromFile(L"./Shaders/BasicShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &pVertexShader, nullptr)))
	{
		__debugbreak();
	}
	if (FAILED(D3DCompileFromFile(L"./Shaders/BasicShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &pPixelShader, nullptr)))
	{
		__debugbreak();
	}


	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 32,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};


	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader->GetBufferPointer(), pVertexShader->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader->GetBufferPointer(), pPixelShader->GetBufferSize());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	//psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	if (FAILED(D3D::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState))))
	{
		__debugbreak();
	}

	if (pVertexShader)
	{
		pVertexShader->Release();
		pVertexShader = nullptr;
	}
	if (pPixelShader)
	{
		pPixelShader->Release();
		pPixelShader = nullptr;
	}
	return TRUE;
}

BOOL CObjectTest3::InitConstantBuffer()
{

	const UINT64 VertexBufferSize = sizeof(BasicVertexConstantBuffer);

	const CD3DX12_HEAP_PROPERTIES vertexBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	const CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);

	ThrowIfFailed(D3D::GetDevice()->CreateCommittedResource(
		&vertexBufferHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexConstantBuffer)));

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC vertexDesc = {};
	vertexDesc.BufferLocation = m_vertexConstantBuffer->GetGPUVirtualAddress();
	vertexDesc.SizeInBytes = VertexBufferSize;

	// Map and initialize the constant buffer. We don't unmap this until the
	// app closes. Keeping things mapped for the lifetime of the resource is okay.
	ThrowIfFailed(m_vertexConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_vertexDataBegin)));

	const UINT64 PixelBufferSize = sizeof(BasicPixelConstantBuffer);

	const CD3DX12_HEAP_PROPERTIES PixelBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	const CD3DX12_RESOURCE_DESC PixelBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(PixelBufferSize);

	ThrowIfFailed(D3D::GetDevice()->CreateCommittedResource(
		&PixelBufferHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&PixelBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pixelConstantBuffer)));

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC PixelDesc = {};
	PixelDesc.BufferLocation = m_pixelConstantBuffer->GetGPUVirtualAddress();
	PixelDesc.SizeInBytes = PixelBufferSize;

	// Map and initialize the constant buffer. We don't unmap this until the
	// app closes. Keeping things mapped for the lifetime of the resource is okay.
	ThrowIfFailed(m_pixelConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pixelDataBegin))); 
	
	return TRUE;
}

void CObjectTest3::Mesh()
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

			//cout << meshData.albedoTextureFilename << endl;
			wstring message_w;
			newMesh->texture = new CTexture(message_w.assign(meshData.albedoTextureFilename.begin(), meshData.albedoTextureFilename.end()));

			//delete t;
		}
		newMesh->vertexConstantBuffer = m_vertexConstantBuffer;
		newMesh->pixelConstantBuffer = m_pixelConstantBuffer;

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


void CObjectTest3::TextureMaterial(wstring wchFileName)
{
	HRESULT hr;

	texture = new CTexture(wchFileName);
}

void CObjectTest3::Update()
{
	m_BasicVertexConstantBufferData.model = XMMatrixTranspose(m_matWorld);

	m_BasicVertexConstantBufferData.invTranspose =
		m_BasicVertexConstantBufferData.model;
	m_BasicVertexConstantBufferData.invTranspose.Translation(Vector3(0.0f));
	m_BasicVertexConstantBufferData.invTranspose =
		m_BasicVertexConstantBufferData.invTranspose.Transpose().Invert();

	// 시점 변환
	m_BasicVertexConstantBufferData.view = XMMatrixTranspose(Context::Get()->View());



	m_BasicPixelConstantBufferData.eyeWorld = Vector3::Transform(
		Vector3(0.0f), m_BasicVertexConstantBufferData.view.Invert());

	// 프로젝션
	m_BasicVertexConstantBufferData.projection = XMMatrixTranspose(Context::Get()->Projection());

	m_BasicPixelConstantBufferData.material.diffuse =
		Vector3(0.8f);
	m_BasicPixelConstantBufferData.material.specular =
		Vector3(1.0f);

	// 여러 개 조명 사용 예시
	for (int i = 0; i < 3; i++) {
		// 다른 조명 끄기
		if (i != 0) {
			//m_BasicPixelConstantBufferData.lights[i].Strength *= 0.0f;
		}
		else {
			m_BasicPixelConstantBufferData.lights[i] = m_lightFromGUI;
		}
	}

	memcpy(m_vertexDataBegin + sizeof(BasicVertexConstantBuffer) * D3D::Get()->FrameCount(), &m_BasicVertexConstantBufferData, sizeof(m_BasicVertexConstantBufferData));
	memcpy(m_pixelDataBegin + sizeof(BasicPixelConstantBuffer) * D3D::Get()->FrameCount(), &m_BasicPixelConstantBufferData, sizeof(m_BasicPixelConstantBufferData));
	
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

void CObjectTest3::Render()
{
	ID3D12GraphicsCommandList6* pCommandList = D3D::Get()->GetCurrentCommandList();
	UINT srvDescriptorSize = D3D::Get()->INL_GetSrvDescriptorSize();
	ID3D12DescriptorHeap* pDescriptorHeap = D3D::Get()->INL_GetSingleDescriptorAllocator()->GetDescriptorHeap();

	pCommandList->SetPipelineState(m_pPipelineState);
	pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	for (auto obj : m_objects)
	{
		pCommandList->SetGraphicsRootConstantBufferView(3, obj.InstanceResource->GetGPUVirtualAddress() + sizeof(Instance) * D3D::Get()->FrameCount());

		for (auto mesh : obj.Model)
		{
			CD3DX12_GPU_DESCRIPTOR_HANDLE tex(pDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex.Offset(mesh->texture->GetDescriptorOffset(), srvDescriptorSize);
			pCommandList->SetGraphicsRootDescriptorTable(0, tex);


			pCommandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
			pCommandList->IASetIndexBuffer(&mesh->indexBufferView);
			pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			pCommandList->SetGraphicsRootConstantBufferView(1, mesh->vertexConstantBuffer->GetGPUVirtualAddress() + sizeof(BasicVertexConstantBuffer) * D3D::Get()->FrameCount());
			pCommandList->SetGraphicsRootConstantBufferView(2, mesh->pixelConstantBuffer->GetGPUVirtualAddress() + sizeof(BasicPixelConstantBuffer) * D3D::Get()->FrameCount());

			pCommandList->DrawIndexedInstanced(mesh->m_indexCount, 1, 0, 0, 0);
		}


	}
}

void CObjectTest3::Destroy()
{
	Cleanup();
}



void CObjectTest3::SetTrasform(Vector3 vec)
{
	m_matTrans = XMMatrixTranslation(vec.x,vec.y,vec.z);


	m_matWorld = XMMatrixMultiply(m_matScale, m_matRot);
	m_matWorld = XMMatrixMultiply(m_matWorld, m_matTrans);
}



void CObjectTest3::Cleanup()
{
	for (auto mesh : m_meshes)
	{
		mesh->indexBuffer->Release();
		mesh->indexBuffer = nullptr;

		mesh->vertexBuffer->Release();
		mesh->vertexBuffer = nullptr;

		delete mesh->texture;
		mesh->texture = nullptr;

	}

	if (m_pVertexBuffer)
	{
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	}

	if (m_pIndexBuffer)
	{
		m_pIndexBuffer->Release();
		m_pIndexBuffer = nullptr;
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
	if (m_constantBuffer)
	{
		m_constantBuffer->Release();
		m_constantBuffer = nullptr;
	}
	if (m_vertexConstantBuffer)
	{
		m_vertexConstantBuffer->Release();
		m_vertexConstantBuffer = nullptr;
	}
	if (m_pixelConstantBuffer)
	{
		m_pixelConstantBuffer->Release();
		m_pixelConstantBuffer = nullptr;
	}
	if (srv.ptr)
	{
		D3D::Get()->INL_GetSingleDescriptorAllocator()->FreeDescriptorHandle(srv);
		srv = {};
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

}

