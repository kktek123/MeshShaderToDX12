#include "pch.h"
#include "D3D.h"
#include "ResourceManager.h"
#include "DescriptorAllocator.h"
#include "Context.h"
#include "GeometryGenerator.h"
#include <DDSTextureLoader.h>
#include <fp16.h>
#include <assert.h>
#include "LandObject.h"
#include "Texture.h"
#include "Mouse.h"
#include "Keyboard.h"

void CLandObject::Init()
{
	InitRootSinagture();
	InitPipelineState();
	//Mesh();
	m_matScale = XMMatrixIdentity();
	m_matRot = XMMatrixIdentity();
	m_matTrans = XMMatrixIdentity();
	m_matWorld = XMMatrixIdentity();
	//m_matRot = XMMatrixRotationX(XM_PIDIV2);
	SetTrasform(Vector3(0, 0, 0));

	m_dwInitRefCount++;

}

BOOL CLandObject::InitRootSinagture()
{
	CD3DX12_DESCRIPTOR_RANGE ranges[1] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	// b0 : Constant Buffer View

	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsConstantBufferView(0);

	// default sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	//SetDefaultSamplerDesc(&sampler, 0);
	//pOutSamperDesc->Filter = D3D12_FILTER_ANISOTROPIC;
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

	const UINT64 constantBufferSize = sizeof(SceneConstantBuffer);

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
	ThrowIfFailed(m_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_cbvDataBegin)));


	return TRUE;
}

BOOL CLandObject::InitPipelineState()
{
	ID3D12Device5* pD3DDeivce = D3D::GetDevice();

	ID3DBlob* pVertexShader = nullptr;
	ID3DBlob* pPixelShader = nullptr;


#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	if (FAILED(D3DCompileFromFile(L"./Shaders/shBasicMesh.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &pVertexShader, nullptr)))
	{
		__debugbreak();
	}
	if (FAILED(D3DCompileFromFile(L"./Shaders/shBasicMesh.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pPixelShader, nullptr)))
	{
		__debugbreak();
	}


	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,	0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 32,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
	if (FAILED(pD3DDeivce->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState))))
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

float GetHillsHeight(float x, float z)
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 GetHillsNormal(float x, float z)
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}


void CLandObject::Mesh()
{
	ResourceManager* pResourceManager = D3D::Get()->GetResourceManager();

	width = heightMap->GetWidth();
	height = heightMap->GetHeight();

	MeshData meshData = GeometryGenerator::CreateGrid(20.0f, 30.0f, height, width);
		//GeometryGenerator::MakeSquareGrid(6, 10,10);
	//std::vector<std::uint16_t> indices = meshData.GetIndices16();
	//count = indices.size();
	vector<Color> heights;
	heightMap->ReadPixel(DXGI_FORMAT_R8G8B8A8_UNORM, &heights);
	vertices = meshData.vertices;
	indices = meshData.indices;

	for (UINT z = 0; z < height; z++)
	{
		for (UINT x = 0; x < width; x++)
		{
			UINT index = width * z + x;
			UINT pixel = width * (height - 1 - z) + x;

			vertices[index].position.y = (heights[pixel].R() * 255.0f) / 90.0f;

		}
	}
	//CreateVertexData();
	//CreateIndexData();
	CreateNormalData();

	std::vector<std::uint16_t> indices16 = meshData.GetIndices16();
	count = indices.size();

	if (FAILED(pResourceManager->CreateDynamicVertexBuffer(sizeof(Vertex), (DWORD)vertices.size(), &m_VertexBufferView, &m_pVertexBuffer, &m_pVertexUploadBuffer, vertices.data())))
	{
		__debugbreak();
	}
	if (FAILED(pResourceManager->CreateIndexBuffer((DWORD)indices16.size(), &m_IndexBufferView, &m_pIndexBuffer, indices16.data())))
	{
		__debugbreak();
	}

	//if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(Vertex), (DWORD)vertexCount, &m_VertexBufferView, &m_pVertexBuffer, vertices)))
	//{
	//	__debugbreak();
	//}
	//if (FAILED(pResourceManager->CreateIndexBuffer((DWORD)indexCount, &m_IndexBufferView, &m_pIndexBuffer, indices)))
	//{
	//	__debugbreak();
	//}
	isPicked.resize(vertices.size());
}


void CLandObject::TextureMaterial(wstring file)
{
	alphaMap = new CTexture(file);
}

void CLandObject::SetTrasform(Vector3 vec)
{
	m_matTrans = XMMatrixTranslation(vec.x, vec.y, vec.z);


	m_matWorld = XMMatrixMultiply(m_matScale, m_matRot);
	m_matWorld = XMMatrixMultiply(m_matWorld, m_matTrans);
}

void CLandObject::Update()
{
	Vector3 position;
	if (Keyboard::Get()->Press(VK_SPACE)) {
		if (Mouse::Get()->Press(0)) {
			position = GetPickedPosition ();
			RaiseHeight(position, brushDesc.Type, brushDesc.Range);
			//bpress = true;
		}
		else
		{
			position = GetPickedPosition();
			brushDesc.Location = position;

		}

	}
	D3D::Get()->CheckPosition = brushDesc.Location;

	m_matWorld = XMMATRIX(g_XMIdentityR0, g_XMIdentityR1, g_XMIdentityR2, g_XMIdentityR3);
	XMMATRIX view = XMLoadFloat4x4(&Context::Get()->View());
	XMMATRIX proj = XMLoadFloat4x4(&Context::Get()->Projection());


	m_constantBufferData.matWorld = XMMatrixTranspose(m_matWorld);
	m_constantBufferData.matView = XMMatrixTranspose(view);
	m_constantBufferData.matProj = XMMatrixTranspose(proj);
	memcpy(m_cbvDataBegin + sizeof(SceneConstantBuffer) * D3D::Get()->FrameCount(), &m_constantBufferData, sizeof(m_constantBufferData));

}

void CLandObject::Render()
{
	//D3D::Get()->GetRenderer()->RenderObjectPush(this);
	ID3D12GraphicsCommandList6* ppCommandList = D3D::Get()->GetCurrentCommandList();
	ProcessRender(ppCommandList, 0);

}

void CLandObject::ProcessRender(ID3D12GraphicsCommandList* pCommandList, DWORD ThreadIndex)
{
	UINT srvDescriptorSize = D3D::Get()->INL_GetSrvDescriptorSize();
	ID3D12DescriptorHeap* pDescriptorHeap = D3D::Get()->INL_GetSingleDescriptorAllocator()->GetDescriptorHeap();

	D3D::Get()->SetRenderTarget(NULL, NULL, ThreadIndex);


	// set RootSignature
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);

	// 이번에 사용할 constant buffer의 descriptor를 렌더링용(shader visible) descriptor table에 카피


	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(pDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	tex.Offset(alphaMap->GetDescriptorOffset(), srvDescriptorSize);

	pCommandList->SetGraphicsRootDescriptorTable(0, tex);
	pCommandList->SetGraphicsRootConstantBufferView(1, m_constantBuffer->GetGPUVirtualAddress() + sizeof(SceneConstantBuffer) * D3D::Get()->FrameCount());

	pCommandList->SetPipelineState(m_pPipelineState);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	pCommandList->IASetIndexBuffer(&m_IndexBufferView);
	pCommandList->DrawIndexedInstanced(count, 1, 0, 0, 0);
}

void CLandObject::Destroy()
{
	Cleanup();
}


void CLandObject::Cleanup()
{
	if (m_pVertexBuffer)
	{
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	}
	if (m_pVertexUploadBuffer)
	{
		m_pVertexUploadBuffer->Release();
		m_pVertexUploadBuffer = nullptr;
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
	if (srv.ptr)
	{
		D3D::Get()->INL_GetSingleDescriptorAllocator()->FreeDescriptorHandle(srv);
		srv = {};
	}
	if (baseMap)
	{
		delete baseMap;
		baseMap = nullptr;
	}
	if (layerMap)
	{
		delete layerMap;
		layerMap = nullptr;
	}
	if (layerMap1)
	{
		delete layerMap1;
		layerMap1 = nullptr;
	}
	if (layerMap2)
	{
		delete layerMap2;
		layerMap2 = nullptr;
	}
	if (alphaMap)
	{
		delete alphaMap;
		alphaMap = nullptr;
	}
	if (normalMap)
	{
		delete normalMap;
		normalMap = nullptr;
	}
	if (specularMap)
	{
		delete specularMap;
		specularMap = nullptr;
	}
	if (detailMap)
	{
		delete detailMap;
		detailMap = nullptr;
	}
	if (snowMap)
	{
		delete snowMap;
		snowMap = nullptr;
	}
	if (distanceMap)
	{
		delete distanceMap;
		distanceMap = nullptr;
	}
	if (heightMap)
	{
		delete heightMap;
		heightMap = nullptr;
	}
}

void CLandObject::BaseMap(wstring file)
{
}

void CLandObject::HeightMap(wstring file)
{
	heightMap = new CTexture(file);

}

void CLandObject::LayerMap(wstring file, wstring alphaFile)
{
}

void CLandObject::AlphaMap(wstring alpha)
{
	alphaMap = new CTexture(alpha);
}

void CLandObject::LayerMap1(wstring layer)
{
}

void CLandObject::LayerMap2(wstring layer)
{
}

void CLandObject::LayerMap3(wstring layer)
{
}

void CLandObject::NormalMap(wstring file)
{
}

void CLandObject::SpecularMap(wstring file)
{
}

void CLandObject::DetailMap(wstring file)
{
}

void CLandObject::SnowMap(wstring file)
{
}

void CLandObject::DistanceMap(wstring file)
{
}

float CLandObject::GetHeight(Vector3& position)
{
	return 0.0f;
}

float CLandObject::GetHeightPick(Vector3& position)
{
	return 0.0f;
}

Vector3 CLandObject::GetPickedPosition()
{
	Matrix V = Context::Get()->View();
	Matrix P = Context::Get()->Projection();
	Matrix W = m_matWorld;// transform->World();
	Viewport* Vp = Context::Get()->GetViewport();

	Vector3 mouse = Mouse::Get()->GetPosition();
	float vx = (+2.0f * mouse.x / D3D::GetDesc().Width - 1.0f) / P(0, 0);
	float vy = (-2.0f * mouse.y / D3D::GetDesc().Height + 1.0f) / P(1, 1);


	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);
	XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);
	XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);

	rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
	rayDir = XMVector3TransformNormal(rayDir, toLocal);

	// Make the ray direction unit length for the intersection tests.
	rayDir = XMVector3Normalize(rayDir);


	Vector3 n, f;
	mouse.z = 0.0f;
	n = Vp->Unproject(mouse, P, V, W);

	mouse.z = 1.0f;
	f = Vp->Unproject(mouse, P, V, W);
	Ray ray;
	ray.position = rayOrigin;
	ray.direction = rayDir;

	for (UINT z = 0; z < height - 1; z++)
	{
		for (UINT x = 0; x < width - 1; x++)
		{
			UINT index[4];
			index[0] = width * z + x;
			index[1] = width * (z + 1) + x;
			index[2] = width * z + x + 1;
			index[3] = width * (z + 1) + x + 1;

			Vector3 p[4];
			for (int i = 0; i < 4; i++)
				p[i] = vertices[index[i]].position;

			Plane plane1(p[0], p[1], p[2]);
			Plane plane2(p[3], p[1], p[2]);
			float distance = 0;
			//if (ray.Intersects(plane1, distance) == TRUE)
			if (TriangleTests::Intersects(rayOrigin, rayDir, p[0], p[1], p[2], distance)) {
				PickIndex = index[0];
				return p[0] + (p[1] - p[0]) + (p[2] - p[0]);
			}
			//if (ray.Intersects(plane2, distance) == TRUE)
			if (TriangleTests::Intersects(rayOrigin, rayDir, p[3], p[1], p[2], distance)) {
				PickIndex = index[3];
 				return p[3] + (p[1] - p[3]) + (p[2] - p[3]);
			}

			//if (D3DXIntersectTri(&p[3], &p[1], &p[2], &start, &direction, &u, &v, &distance) == TRUE)
			//	return p[3] + (p[1] - p[3]) * u + (p[2] - p[3]) * v;
		}
	}

	return Vector3(-1, FLT_MIN, -1);
}

void CLandObject::RaiseHeight(Vector3& position, UINT type, UINT range)
{
	D3D12_RECT rect;
	Vector3 p = vertices[PickIndex].position;
	//vector<bool> isPicked(vertices.size());
	rect.left = p.x - range;
	rect.top = p.z + range;
	rect.right = p.x + range;
	rect.bottom = p.z - range;

	//if (rect.left < 0)			rect.left = 0;
	if (rect.top >= height)		rect.top = height;
	if (rect.right >= width)	rect.right = width;
	//if (rect.bottom < 0)		rect.bottom = 0;
	if (alpha_pixel_color.size() <= 0)
	heightMap->ReadPixel(DXGI_FORMAT_R8G8B8A8_UNORM, &alpha_pixel_color);

 	for (int z = rect.bottom; z <= rect.top; z++)
	{
		for (int x = rect.left; x <= rect.right; x++)
		{
			UINT index = PickIndex + width * z + x;
			if (index >= vertices.size())
				continue;
			float speed;
			float ax = (x - p.x);
			if (ax < 0)ax = -ax;
			float az = (z - p.z);
			if (az < 0)az = -az;
			speed = 5.0f / (2 * ax + 2 * az + 1.0f);
			if (type == 1)
			{
				float dx = abs(vertices[index].position.x - p.x) * 15;
				float dz = abs(vertices[index].position.z - p.z) * 15;
				//float dist = sqrt(dx * dx + dz * dz) ;
				if (dx <= range && dz <= range)
				{
					isPicked[index] = true;
					vertices[index].position.y += 5.0f * 0.001;//Time::Delta();
				}
			}
			else if (type == 2)
			{
				float dx = vertices[index].position.x - p.x;
				float dz = vertices[index].position.z - p.z;
				float dist = sqrt(dx * dx + dz * dz)* 15;
				if (dist <= range)
				{
					//isPicked[index] = true;
					vertices[index].position.y += 5.0f * 0.001;//Time::Delta();
				}
			}
			else if (type == 3)
			{
 				float dx = vertices[index].position.x - p.x;
				float dz = vertices[index].position.z - p.z;
				float dist = sqrt(dx * dx + dz * dz) * 15;
				//for (float i = 0; i < range-2.0f; i++)
				//{
				//	if (dist <= range-i)
				//	{
				//		vertices[index].Position.y += (0.5f+i/(range+i)) * Time::Delta();
				//	}
				//}
				if (dist <= range)
				{
					float angle = acosf(dist / range);
					float factor = sinf(angle);
					//(dist * radian > range ? 1.0f : dist * radian / range);
					vertices[index].position.y += 5.0f * 0.001 * factor;//Time::Delta() * (factor);
				}
			}
		}
	}
	 ResourceManager * pResourceManager = D3D::Get()->GetResourceManager();
	if (FAILED(pResourceManager->UpdateVertexBuffer(sizeof(Vertex), (DWORD)vertices.size(), &m_VertexBufferView, m_pVertexBuffer, m_pVertexUploadBuffer, vertices.data())))
	{
		__debugbreak();
	}

	//UINT8* pVertexDataBegin = nullptr;


}

void CLandObject::CreateTestTexture(wstring file)
{
}

void CLandObject::LayerBrush(Vector3& position, UINT range, UINT type)
{
}

void CLandObject::DrawTextureBrush(Vector3& position, UINT range, UINT type)
{
}

void CLandObject::SaveXmlfile(wstring file)
{
}

void CLandObject::Loadfile(wstring file)
{
}

void CLandObject::OpenTexture(wstring filename)
{
}

void CLandObject::CreateVertexData()
{
	//vector<Color> heights;
	//heightMap->ReadPixel(DXGI_FORMAT_R8G8B8A8_UNORM, &heights);

	//width = heightMap->GetWidth();
	//height = heightMap->GetHeight();


	//vertexCount = width * height;
	//vertices = new Vertex[vertexCount];
	//for (UINT z = 0; z < height; z++)
	//{
	//	for (UINT x = 0; x < width; x++)
	//	{
	//		UINT index = width * z + x;
	//		UINT pixel = width * (height - 1 - z) + x;

	//		vertices[index].position.x = (float)x;
	//		vertices[index].position.y = (heights[pixel].R() * 255.0f) / 3.0f;
	//		vertices[index].position.z = (float)z;

	//		vertices[index].texcoord.x = (float)x / (float)width;
	//		vertices[index].texcoord.y = (float)(height - 1 - z) / (float)height;
	//		//vertices[index].Color = Color(1.f, 1.f, 1.f, 1.f);
	//	}
	//}

}

void CLandObject::CreateIndexData()
{
	//indexCount = (width - 1) * (height - 1) * 6;
	//indices = new UINT[indexCount];

	//UINT index = 0;
	//for (UINT z = 0; z < height - 1; z++)
	//{
	//	for (UINT x = 0; x < width - 1; x++)
	//	{
	//		indices[index + 0] = width * z + x;
	//		indices[index + 1] = width * (z + 1) + x;
	//		indices[index + 2] = width * z + x + 1;
	//		indices[index + 3] = width * z + x + 1;
	//		indices[index + 4] = width * (z + 1) + x;
	//		indices[index + 5] = width * (z + 1) + x + 1;

	//		index += 6;
	//	}
	//}
}

void CLandObject::CreateNormalData()
{
	for (UINT i = 0; i < indexCount / 3; i++)
	{
		UINT index0 = indices[i * 3 + 0];
		UINT index1 = indices[i * 3 + 1];
		UINT index2 = indices[i * 3 + 2];

		Vertex v0 = vertices[index0];
		Vertex v1 = vertices[index1];
		Vertex v2 = vertices[index2];

		//NormalVector
		{
			Vector3 d1 = v1.position - v0.position;
			Vector3 d2 = v2.position - v0.position;

			Vector3 normal = DirectX::XMVector3Cross(d1,d2);

			vertices[index0].normalModel += normal;
			vertices[index1].normalModel += normal;
			vertices[index2].normalModel += normal;
		}

		//TangentVector
		{
			Vector3 d1 = v1.position - v0.position;
			Vector3 d2 = v2.position - v0.position;

			Vector2 u = v1.texcoord - v0.texcoord;
			Vector2 v = v2.texcoord - v0.texcoord;

			float d = 1.0f / (u.x * v.y - u.y * v.x);

			Vector3 tangent;
			tangent.x = (v.y * d1.x - v.x * d2.x) * d;
			tangent.y = (v.y * d1.y - v.x * d2.y) * d;
			tangent.z = (v.y * d1.z - v.x * d2.z) * d;

			vertices[index0].tangentModel += tangent;
			vertices[index1].tangentModel += tangent;
			vertices[index2].tangentModel += tangent;
		}
	}

	for (UINT i = 0; i < vertexCount; i++)
	{
		DirectX::XMVector3Normalize(vertices[i].normalModel);
		DirectX::XMVector3Normalize(vertices[i].tangentModel);
	}

}
