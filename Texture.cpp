#include "pch.h"
#include "D3D.h"
#include "./Utilities/Path.h"
#include "ResourceManager.h"
#include "DescriptorAllocator.h"
#include "Context.h"
#include "DirectXHelpers.h"
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include "Texture.h"

vector<TextureDesc> CTextures::descs;
#pragma comment(lib,"DirectXTex.lib")
CTexture::CTexture(wstring file)
{
	//bool b = Path::IsRelativePath(file);
	//if (b == true)
	//	this->file = L"../../_Textures/" + file;
	this->file = file;
	CTextures::Load(this);
	//String::Replace(&this->file, L"../../_Textures", L"");

}


void CTexture::SaveFile(wstring file)
{
	ID3D12Resource* srcTexture;

	//srcTexture = view.ptr;
	//D3D::GetDevice()->GetResourceTiling
	SaveFile(file, pTexResource);
}

void CTexture::SaveFile(wstring file, ID3D12Resource* src)
{
}

void CTexture::ReadPixel(DXGI_FORMAT readFormat, vector<Color>* pixels)
{
	ReadPixel(pUploadBuffer, readFormat, pixels);
}

void CTexture::ReadPixel(ID3D12Resource* src, DXGI_FORMAT readFormat, vector<Color>* pixels)
{

	BYTE* pMappedPtr = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	src->Map(0, &readRange, reinterpret_cast<void**>(&pMappedPtr));


	UINT* colors = (UINT*)pMappedPtr;


	src->Unmap(0, &readRange);

	for (UINT y = 0; y < Height; y++)
	{
		for (UINT x = 0; x < Width; x++)
		{
			UINT index = Width * y + x;

			float f = 1.0f / 255.0f;
			float r = f * (float)((0xFF000000 & colors[index]) >> 24);
			float g = f * (float)((0x00FF0000 & colors[index]) >> 16);
			float b = f * (float)((0x0000FF00 & colors[index]) >> 8);
			float a = f * (float)((0x000000FF & colors[index]) >> 0);

			pixels->push_back(Color(a, b, g, r));
		}
	}

	//delete[] pMappedPtr;
	//delete[] pMappedPtr;
	//SafeDeleteArray(colors);
	//SafeRelease(src);

}


CTexture::~CTexture()
{
	if (pTexResource)
	{
		pTexResource->Release();
		pTexResource = nullptr;
	}
	if (pUploadBuffer)
	{
		pUploadBuffer->Release();
		pUploadBuffer = nullptr;
	}



	if (view.ptr)
	{
		view = {};
	}

}


ID3D12Resource* CTexture::GetTexture()
{
	return pTexResource;
}

void CTextures::Create()
{
}

void CTextures::Delete()
{
	for (TextureDesc temp : CTextures::descs)
	{
		D3D::Get()->INL_GetSingleDescriptorAllocator()->FreeDescriptorHandle(temp.view);
	}
}

void CTextures::Load(CTexture* texture)
{
	HRESULT hr;

	TexMetadata metaData;
	wstring ext = Path::GetExtension(texture->file);
	if (ext == L"tga")
	{
		hr = GetMetadataFromTGAFile(texture->file.c_str(), metaData);
		assert(SUCCEEDED(hr));
	}
	else if (ext == L"dds")
	{
		hr = GetMetadataFromDDSFile(texture->file.c_str(), DDS_FLAGS_NONE, metaData);
		assert(SUCCEEDED(hr));
	}
	else if (ext == L"hdr")
	{
		assert(false);
	}
	else
	{
		hr = GetMetadataFromWICFile(texture->file.c_str(), WIC_FLAGS_NONE, metaData);
		assert(SUCCEEDED(hr));
	}

	UINT width = metaData.width;
	UINT height = metaData.height;

	TextureDesc desc;
	desc.file = texture->file;
	desc.width = width;
	desc.height = height;
	TextureDesc exist;
	bool bExist = false;
	for (TextureDesc temp : descs)
	{
		if (desc == temp)
		{
			bExist = true;
			exist = temp;

			break;
		}
	}

	if (bExist == true)
	{
		texture->metaData = exist.metaData;
		texture->view = exist.view;
		texture->DescriptorOffset = exist.DescriptorOffset;
		texture->Width = exist.width;
		texture->Height = exist.height;
	}
	else
	{
		ScratchImage image;
		if (ext == L"tga")
		{
			hr = LoadFromTGAFile(texture->file.c_str(), &metaData, image);
			assert(SUCCEEDED(hr));
		}
		else if (ext == L"dds")
		{
			hr = LoadFromDDSFile(texture->file.c_str(), DDS_FLAGS_NONE, &metaData, image);
			assert(SUCCEEDED(hr));
		}
		else if (ext == L"hdr")
		{
			assert(false);
		}
		else
		{
			hr = LoadFromWICFile(texture->file.c_str(), WIC_FLAGS_NONE, &metaData, image);
			assert(SUCCEEDED(hr));
		}

		ID3D12GraphicsCommandList* m_pCommandList = D3D::Get()->GetResourceManager()->GetCommandList();
		ID3D12CommandQueue* m_pCommandQueue = D3D::Get()->GetResourceManager()->GetCommandQueue();
		ID3D12CommandAllocator* m_pCommandAllocator = D3D::Get()->GetResourceManager()->GetCommandAllocator();

		//if (IsSupportedTexture(D3D::GetDevice(), image.GetMetadata()))
		//{
			//hr = CreateTexture(D3D::GetDevice(), image.GetMetadata(), &texture->pTexResource);
		//}


		//hr = DirectX::PrepareUpload(D3D::GetDevice(),
		//	image.GetImages(),
		//	image.GetImageCount(),
		//	image.GetMetadata(),
		//	subResources);
		vector<D3D12_SUBRESOURCE_DATA> subResources;
			std::unique_ptr<uint8_t[]> ddsData;
		if (ext == L"dds")
		{
			hr = LoadDDSTextureFromFile(D3D::GetDevice(), texture->file.c_str(), &texture->pTexResource, ddsData, subResources);
			assert(SUCCEEDED(hr));
		}
		else
		{
			subResources.resize(1);
			hr = LoadWICTextureFromFile(D3D::GetDevice(), texture->file.c_str(), &texture->pTexResource, ddsData, subResources[0]);
			assert(SUCCEEDED(hr));
		}


		const UINT64 uploadBufferSize =
			GetRequiredIntermediateSize(texture->pTexResource, 0, subResources.size());


		hr = D3D::GetDevice()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&texture->pUploadBuffer));


		if (FAILED(hr))
			__debugbreak();

		if (FAILED(m_pCommandAllocator->Reset()))
			__debugbreak();

		if (FAILED(m_pCommandList->Reset(m_pCommandAllocator, nullptr)))
			__debugbreak();

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->pTexResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

		UpdateSubresources(m_pCommandList,
			texture->pTexResource, texture->pUploadBuffer,
			0, 0, subResources.size(),
			&subResources[0]);

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->pTexResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));

		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		D3D::Get()->GetResourceManager()->Fence();
		D3D::Get()->GetResourceManager()->WaitForFenceValue();


		D3D12_CPU_DESCRIPTOR_HANDLE view = {};

		DXGI_FORMAT TexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = texture->pTexResource->GetDesc().Format;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = texture->pTexResource->GetDesc().MipLevels;

		//texture->pUploadBuffer->Release();
		//texture->pUploadBuffer = nullptr;


		if (D3D::Get()->INL_GetSingleDescriptorAllocator()->AllocDescriptorHandle(&view))
		{
			 D3D::GetDevice()->CreateShaderResourceView(texture->pTexResource, &SRVDesc, view);
			
			 //CreateShaderResourceView(D3D::GetDevice(), texture->pTexResource, view);
		}
		else
		{
			texture->pTexResource->Release();
			texture->pTexResource = nullptr;
		}

		desc.file = texture->file;
		desc.width = metaData.width;
		desc.height = metaData.height;
		desc.view = view;
		desc.metaData = metaData;
		desc.DescriptorOffset = descs.size();

		texture->view = view;
		texture->metaData = metaData;
		texture->Width = metaData.width;
		texture->Height = metaData.height;
		texture->DescriptorOffset = descs.size();

		descs.push_back(desc);
	}

}

CTextureArray::CTextureArray(vector<wstring>& names, UINT width, UINT height, UINT mipLevels)
{
}

CTextureArray::~CTextureArray()
{
}

vector<ID3D12Resource*> CTextureArray::CreateTextures(vector<wstring>& names, UINT width, UINT height, UINT mipLevels)
{
	return vector<ID3D12Resource*>();
}
