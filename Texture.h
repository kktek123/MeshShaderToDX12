#pragma once

class CTexture
{
public:
	friend class CTextures;

public:
	void SaveFile(wstring file);
	static void SaveFile(wstring file, ID3D12Resource* src);

	void ReadPixel(DXGI_FORMAT readFormat, vector<Color>* pixels);
	void ReadPixel(ID3D12Resource* src, DXGI_FORMAT readFormat, vector<Color>* pixels);

public:
	CTexture(wstring file);//, D3DX11_IMAGE_LOAD_INFO* loadInfo = NULL);
	~CTexture();

	operator D3D12_CPU_DESCRIPTOR_HANDLE () { return view; }


	wstring GetFile() { return file; }

	UINT GetWidth() { return metaData.width; }
	UINT GetHeight() { return metaData.height; }

	void GetImageInfo(DirectX::TexMetadata* data)
	{
		*data = metaData;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE SRV() { return view; }
	UINT GetDescriptorOffset() { return DescriptorOffset; }
	ID3D12Resource* GetTexture();

private:
	wstring file;

	DirectX::TexMetadata metaData;
	ID3D12Resource* pTexResource;
	D3D12_CPU_DESCRIPTOR_HANDLE view;
	ID3D12Resource* pUploadBuffer = nullptr;
	UINT Width;
	UINT Height;
	UINT DescriptorOffset;
};

struct TextureDesc
{
	wstring file;
	UINT width, height;
	DirectX::TexMetadata metaData;
	D3D12_CPU_DESCRIPTOR_HANDLE view;
	UINT DescriptorOffset;
	bool operator==(const TextureDesc& desc)
	{
		bool b = true;
		b &= file == desc.file;
		b &= width == desc.width;
		b &= height == desc.height;

		return b;
	}
};

class CTextures
{
public:
	friend class CTexture;

public:
	static void Create();
	static void Delete();

private:
	static void Load(CTexture* texture);// , D3DX11_IMAGE_LOAD_INFO* loadInfo = NULL);

private:
	static vector<TextureDesc> descs;
	ID3D12Resource* pUploadBuffer = nullptr;

};

class CTextureArray
{
public:
	CTextureArray(vector<wstring>& names, UINT width = 256, UINT height = 256, UINT mipLevels = 1);
	~CTextureArray();

	D3D12_CPU_DESCRIPTOR_HANDLE* SRV() { return srv; }

private:
	vector<ID3D12Resource*> CreateTextures(vector<wstring>& names, UINT width, UINT height, UINT mipLevels);

private:
	D3D12_CPU_DESCRIPTOR_HANDLE* srv;

};