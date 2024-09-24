//#include "pch.h"
//#include "D3D.h"
//#include"VertexLayouts.h"
//#include "GeometryGenerator.h"
//#include "ResourceManager.h"
//#include "DescriptorAllocator.h"
//#include "DescriptorPool.h"
//#include "CommandListPool.h"
//#include "SimpleConstantBufferPool.h"
//#include "Renderer.h"
//#include "Context.h"
//#include "GeometryGenerator.h"
//#include <DDSTextureLoader.h>
//#include "Mesh.h"
//
//
//
//void BasicMesh::Initialize(ResourceManager* resourceManager,const std::string& basePath, const std::string& filename)
//{
//	//auto meshes = GeometryGenerator::ReadFromFile(basePath, filename);
//
//	//Initialize(resourceManager,meshes);
//
//}
//
//void BasicMesh::Initialize(ResourceManager* resourceManager, const std::vector<MeshData>& meshes)
//{
//	if (!InitRootSinagture())
//		__debugbreak();
//	ID3D12GraphicsCommandList* m_pCommandList = resourceManager->GetCommandList();
//	ID3D12CommandQueue* m_pCommandQueue = resourceManager->GetCommandQueue();
//	ID3D12CommandAllocator* m_pCommandAllocator = resourceManager->GetCommandAllocator();
//	
//	std::unique_ptr<uint8_t[]> ddsData;
//	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
//
//    for (const auto& meshData : meshes) {
//        auto newMesh = std::make_shared<Mesh>();
//
//        if (!meshData.albedoTextureFilename.empty()) {
//            if (FAILED(LoadDDSTextureFromFile(D3D::GetDevice(), meshData.albedoTextureFilename.c_str(), &newMesh->albedoTexture, ddsData, subresources)))
//            {
//                __debugbreak();
//            }
//        }
//
//        if (!meshData.normalTextureFilename.empty()) {
//        }
//
//        if (!meshData.heightTextureFilename.empty()) {
//        }
//
//        if (!meshData.aoTextureFilename.empty()) {
//        }
//
//        if (!meshData.metallicTextureFilename.empty()) {
//        }
//
//        if (!meshData.roughnessTextureFilename.empty()) {
//        }
//        UINT subresoucesize = (UINT)subresources.size();
//
//        // upload is implemented by application developer. Here's one solution using <d3dx12.h>
//        const UINT64 uploadBufferSize =
//            GetRequiredIntermediateSize(newMesh->albedoTexture, 0, subresoucesize);
//
//        HRESULT hr;
//        hr = D3D::GetDevice()->CreateCommittedResource(
//            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
//            D3D12_HEAP_FLAG_NONE,
//            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
//            D3D12_RESOURCE_STATE_GENERIC_READ,
//            nullptr,
//            IID_PPV_ARGS(&newMesh->albedotextureUploadHeap));
//
//
//
//        if (FAILED(hr))
//            __debugbreak();
//
//        if (FAILED(m_pCommandAllocator->Reset()))
//            __debugbreak();
//
//        if (FAILED(m_pCommandList->Reset(m_pCommandAllocator, nullptr)))
//            __debugbreak();
//
//        m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(newMesh->albedoTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
//
//        UpdateSubresources(m_pCommandList,
//            newMesh->albedoTexture, newMesh->albedotextureUploadHeap,
//            0, 0, subresoucesize,
//            &subresources[0]);
//        m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(newMesh->albedoTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
//
//        m_pCommandList->Close();
//
//        ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
//        m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
//
//        D3D::Get()->GetResourceManager()->Fence();
//        D3D::Get()->GetResourceManager()->WaitForFenceValue();
//
//
//        DXGI_FORMAT TexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
//        D3D12_RESOURCE_DESC	desc = newMesh->albedoTexture->GetDesc();
//
//        D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
//        SRVDesc.Format = desc.Format;
//        SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
//        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
//        SRVDesc.Texture2D.MipLevels = desc.MipLevels;
//
//
//        if (D3D::Get()->INL_GetSingleDescriptorAllocator()->AllocDescriptorHandle(&srv))
//        {
//            D3D::GetDevice()->CreateShaderResourceView(newMesh->albedoTexture, &SRVDesc, srv);
//        }
//        //else
//        //{
//        //    newMesh->albedoTexture->Release();
//        //    newMesh->albedoTexture = nullptr;
//        //}
//
//
//        //newMesh->vertexConstantBuffer = m_vertexConstantBuffer;
//        //newMesh->pixelConstantBuffer = m_pixelConstantBuffer;
//
//        this->m_meshes.push_back(newMesh);
//    }
//
//    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
//        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
//         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
//         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
//         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32,
//         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//    };
//
//    //D3D11Utils::CreateVertexShaderAndInputLayout(
//    //    device, L"BasicVS.hlsl", basicInputElements, m_basicVertexShader,
//    //    m_basicInputLayout);
//
//    //D3D11Utils::CreatePixelShader(device, L"BasicPS.hlsl", m_basicPixelShader);
//
//    //// Geometry shader 초기화하기
//    //D3D11Utils::CreateGeometryShader(device, L"NormalGS.hlsl",
//    //    m_normalGeometryShader);
//
//    //D3D11Utils::CreateVertexShaderAndInputLayout(
//    //    device, L"NormalVS.hlsl", basicInputElements, m_normalVertexShader,
//    //    m_basicInputLayout);
//    //D3D11Utils::CreatePixelShader(device, L"NormalPS.hlsl",
//    //    m_normalPixelShader);
//
//    //D3D11Utils::CreateConstBuffer(device, m_normalVertexConstData,
//    //    m_normalVertexConstantBuffer);
//
//    ID3DBlob* pVertexShader = nullptr;
//    ID3DBlob* pPixelShader = nullptr;
//
//
//#if defined(_DEBUG)
//    // Enable better shader debugging with the graphics debugging tools.
//    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
//#else
//    UINT compileFlags = 0;
//#endif
//    if (FAILED(D3DCompileFromFile(L"./Shaders/shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &pVertexShader, nullptr)))
//    {
//        __debugbreak();
//    }
//    if (FAILED(D3DCompileFromFile(L"./Shaders/shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pPixelShader, nullptr)))
//    {
//        __debugbreak();
//    }
//
//    // Describe and create the graphics pipeline state object (PSO).
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
//    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
//    psoDesc.pRootSignature = m_pRootSignature;
//    psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShader->GetBufferPointer(), pVertexShader->GetBufferSize());
//    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShader->GetBufferPointer(), pPixelShader->GetBufferSize());
//    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
//    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//    psoDesc.DepthStencilState.StencilEnable = FALSE;
//    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
//    psoDesc.SampleMask = UINT_MAX;
//    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//    psoDesc.NumRenderTargets = 1;
//    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
//    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
//    psoDesc.SampleDesc.Count = 1;
//    if (FAILED(D3D::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState))))
//    {
//        __debugbreak();
//    }
//
//    if (pVertexShader)
//    {
//        pVertexShader->Release();
//        pVertexShader = nullptr;
//    }
//    if (pPixelShader)
//    {
//        pPixelShader->Release();
//        pPixelShader = nullptr;
//    }
//
//
//}
//
//void BasicMesh::UpdateConstantBuffers(CSimpleConstantBufferPool* ConstantBufferPool)
//{
//}
//
//void BasicMesh::UpdateModelWorld(const Matrix& modelToWorldRow)
//{
//    m_matWorld = modelToWorldRow;
//}
//
//void BasicMesh::Render()
//{
//    DWORD ThreadIndex = 0;
//    ID3D12GraphicsCommandList* pCommandList = D3D::Get()->GetCommandListPool(ThreadIndex)->GetCurrentCommandList();
//    auto srvDescriptorSize = D3D::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//    CDescriptorPool* pDescriptorPool = D3D::Get()->INL_GetDescriptorPool();
//    ID3D12DescriptorHeap* pDescriptorHeap = pDescriptorPool->INL_GetDescriptorHeap();
//    CSimpleConstantBufferPool* pConstantBufferPool = D3D::Get()->INL_GetConstantBufferPool(ThreadIndex);
//
//    D3D::Get()->SetRenderTarget(NULL, NULL);
//
//    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};
//    CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
//
//    if (!pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, 2))
//    {
//        __debugbreak();
//    }
//
//    CB_CONTAINER* pCB = pConstantBufferPool->Alloc();
//    if (!pCB)
//    {
//        __debugbreak();
//    }
//    CONSTANT_BUFFER_DEFAULT* pConstantBufferDefault = (CONSTANT_BUFFER_DEFAULT*)pCB->pSystemMemAddr;
//
//    // constant buffer의 내용을 설정
//    // view/proj matrix
//    pConstantBufferDefault->matView = Context::Get()->View().Transpose();
//
//    pConstantBufferDefault->matProj = Context::Get()->Projection().Transpose();
//
//    // world matrix
//    pConstantBufferDefault->matWorld = m_matWorld.Transpose();
//
//    // set RootSignature
//    pCommandList->SetGraphicsRootSignature(m_pRootSignature);
//    pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);
//
//
//    // 이번에 사용할 constant buffer의 descriptor를 렌더링용(shader visible) descriptor table에 카피
//    CD3DX12_CPU_DESCRIPTOR_HANDLE Dest(cpuDescriptorTable, 0, srvDescriptorSize);
//    D3D::GetDevice()->CopyDescriptorsSimple(1, Dest, pCB->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	// cpu측 코드에서는 cpu descriptor handle에만 write가능
//
//    for (const auto& mesh : m_meshes)
//    {
//        if (mesh->srv.ptr)
//        {
//            CD3DX12_CPU_DESCRIPTOR_HANDLE Dest(cpuDescriptorTable, 1, srvDescriptorSize);
//            D3D::GetDevice()->CopyDescriptorsSimple(1, Dest, mesh->srv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);	// cpu측 코드에서는 cpu descriptor handle에만 write가능
//        }
//        else
//        {
//            __debugbreak();
//        }
//
//        Dest.Offset(1, srvDescriptorSize);
//
//    }
//    pCommandList->SetPipelineState(m_pPipelineState);
//    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//
//
//    pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);
//
//    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTableForTriGroup(gpuDescriptorTable, DESCRIPTOR_COUNT_PER_OBJ, srvDescriptorSize);
//    for (const auto& mesh : m_meshes) {
//        pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTableForTriGroup);	// Entry of Tri-Groups
//        gpuDescriptorTableForTriGroup.Offset(1, srvDescriptorSize);
//
//        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//        pCommandList->IASetVertexBuffers(0, 1, &mesh->vertexBufferView);
//        pCommandList->IASetIndexBuffer(&mesh->indexBufferView);
//        pCommandList->DrawIndexedInstanced(mesh->m_indexCount, 1, 0, 0, 0);
//
//    }
//
//}
//
//BOOL BasicMesh::InitCommonResources()
//{
//    return 0;
//}
//
//void BasicMesh::CleanupSharedResources()
//{
//}
//
//BOOL BasicMesh::InitRootSinagture()
//{
//    CD3DX12_DESCRIPTOR_RANGE ranges[2] = {};
//    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);	// b0 : Constant Buffer View
//    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);	// t0 : Shader Resource View(Tex)
//
//    CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
//    rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);
//
//    // default sampler
//    D3D12_STATIC_SAMPLER_DESC sampler = {};
//    //SetDefaultSamplerDesc(&sampler, 0);
//    //pOutSamperDesc->Filter = D3D12_FILTER_ANISOTROPIC;
//    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
//
//    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
//    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
//    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
//    sampler.MipLODBias = 0.0f;
//    sampler.MaxAnisotropy = 16;
//    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
//    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
//    sampler.MinLOD = -FLT_MAX;
//    sampler.MaxLOD = D3D12_FLOAT32_MAX;
//    sampler.ShaderRegister = 0;
//    sampler.RegisterSpace = 0;
//    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
//    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
//
//    // Allow input layout and deny uneccessary access to certain pipeline stages.
//    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
//        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
//        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
//        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
//        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
//        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
//
//
//    // Create an empty root signature.
//    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
//    rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//    ID3DBlob* pSignature = nullptr;
//    ID3DBlob* pError = nullptr;
//
//    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError)))
//    {
//        __debugbreak();
//    }
//
//    if (FAILED(D3D::GetDevice()->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))))
//    {
//        __debugbreak();
//    }
//    if (pSignature)
//    {
//        pSignature->Release();
//        pSignature = nullptr;
//    }
//    if (pError)
//    {
//        pError->Release();
//        pError = nullptr;
//    }
//    return TRUE;
//}
//
//BOOL BasicMesh::InitPipelineState()
//{
//    return 0;
//}
//
//void BasicMesh::Cleanup()
//{
//}
