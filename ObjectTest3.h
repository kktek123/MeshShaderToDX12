#pragma once
#include "Object.h"
#include "MathHelper.h"
#include "d3dUtil.h"

class CTexture;

class CObjectTest3 : public CObject
{
public:
	CObjectTest3() {};
	~CObjectTest3() {};
	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;
	virtual void ProcessRender(ID3D12GraphicsCommandList* pCommandList, DWORD ThreadIndex) {};
	virtual void Destroy()override;
	void TextureMaterial(wstring file);
	void SetTrasform(Vector3 vec);
	void Mesh();
protected:

private:
	void	Cleanup();

	BOOL	InitRootSinagture();
	BOOL	InitPipelineState();
	BOOL	InitConstantBuffer();

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;
	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* textureUploadHeap = nullptr;
	DWORD	m_dwInitRefCount = 0;

	// vertex data
	ID3D12Resource* m_pVertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};

	// index data
	ID3D12Resource* m_pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};

	//vector<TextureDesc> descs;

	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	uint32_t count = 0;
	CTexture* texture;

	struct SMesh {

		ID3D12Resource* vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		ID3D12Resource* indexBuffer;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		ID3D12Resource* vertexConstantBuffer;
		ID3D12Resource* pixelConstantBuffer;

		CTexture* texture;
		UINT m_indexCount = 0;
	};

	_declspec(align(256u))struct Instance
	{
		XMFLOAT4X4 World;
		XMFLOAT4X4 WorldInvTrans;
		float    Scale;
		uint32_t     Flags;
	};

	struct SMaterial {
		Vector3 ambient = Vector3(0.1f);  // 12
		float shininess = 8.0f;           // 4
		Vector3 diffuse = Vector3(0.8f);  // 12
		float dummy1;                     // 4
		Vector3 specular = Vector3(0.5f); // 12
		float dummy2;                     // 4
	};

	struct BasicVertexConstantBuffer {
		Matrix model;
		Matrix invTranspose;
		Matrix view;
		Matrix projection;
	};

	struct BasicPixelConstantBuffer {
		Vector3 eyeWorld;         // 12
		bool useTexture = true;          // 4
		SMaterial material;        // 48
		Light lights[3]; // 48 * MAX_LIGHTS
	};


	_declspec(align(256u)) struct SceneConstantBuffer
	{
		XMFLOAT4X4 World;
		XMFLOAT4X4 WorldView;
		XMFLOAT4X4 WorldViewProj;
		XMFLOAT4X4 TexTransform;
		//uint32_t   DrawMeshlets;
	};

	_declspec(align(256u)) struct ConstantMaterialBuffer
	{
		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = 0.25f;

		// Used in texture mapping.
		DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
	};

	_declspec(align(256u)) struct PassConstants
	{
		DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
		float cbPerObjectPad1 = 0.0f;
		DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
		DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
		float NearZ = 0.0f;
		float FarZ = 0.0f;
		float TotalTime = 0.0f;
		float DeltaTime = 0.0f;

		DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

		// Indices [0, NUM_DIR_LIGHTS) are directional lights;
		// indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
		// indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
		// are spot lights for a maximum of MaxLights per object.
		Light Lights[MaxLights];
	};

	ID3D12Resource* m_constantBuffer;
	UINT8* m_cbvDataBegin;
	SceneConstantBuffer m_constantBufferData;

	ID3D12Resource* m_vertexConstantBuffer;
	UINT8* m_vertexDataBegin;
	BasicVertexConstantBuffer m_BasicVertexConstantBufferData;

	ID3D12Resource* m_pixelConstantBuffer;
	UINT8* m_pixelDataBegin;
	BasicPixelConstantBuffer m_BasicPixelConstantBufferData;

	ID3D12DescriptorHeap* mSrvDescriptorHeap = nullptr;


	vector<SMesh*> m_meshes;
	Light m_lightFromGUI;
private:

		struct SceneObject
		{
			vector<SMesh*>			Model;
			DirectX::XMFLOAT4X4		World;
			ID3D12Resource* InstanceResource;
			void* InstanceData;
			UINT8* InstanceDataBegin;
			Instance m_InstanceData;
			uint32_t				Flags;
		};

		inline uint32_t DivRoundUp(uint32_t num, uint32_t den) { return (num + den - 1) / den; }

		vector<SceneObject>           m_objects;

};