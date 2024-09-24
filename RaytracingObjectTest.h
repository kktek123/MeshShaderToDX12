#pragma once
#include "Object.h"
#include "RaytracingHlslCompat.h"
#include "GpuUploadBuffer.h"
#include "StepTimer.h"
#include "d3dUtil.h"
#include "MathHelper.h"

class CTexture;

class CRaytracingObjectTest : public CObject
{
public:
	CRaytracingObjectTest() {};
	~CRaytracingObjectTest() {};
	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;
	virtual void ProcessRender(ID3D12GraphicsCommandList* pCommandList, DWORD ThreadIndex) override;
	virtual void Destroy()override;
	void TextureMaterial(wstring file);
	void SetTrasform(Vector3 vec);
	void Mesh();
protected:

private:
	void	Cleanup();

	BOOL	InitRootSinagture();
	BOOL	InitPipelineState();
	void	InitAccelerationStructures();
	void	InitShaderTables();
	void	CreateRaytracingOutputResource();
	void CopyRaytracingOutputToBackbuffer();
	void UpdateCameraMatrices();
	void CreateConstantBuffers();

	void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ID3D12RootSignature* rootSig);
	UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse = UINT_MAX);
	UINT CreateBufferSRV(D3DBuffer* buffer, UINT numElements, UINT elementSize);
	D3DBuffer m_indexBuffer;
	D3DBuffer m_vertexBuffer;

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12RootSignature* m_raytracingGlobalRootSignature = nullptr;
	ID3D12RootSignature* m_raytracingLocalRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;

	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* textureUploadHeap = nullptr;
	DWORD	m_dwInitRefCount = 0;

	// vertex data
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};

	// index data
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};

	//vector<TextureDesc> descs;

		// Acceleration structure
	ID3D12Resource* m_bottomLevelAccelerationStructure;
	ID3D12Resource* m_topLevelAccelerationStructure;

	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	uint32_t count = 0;
	CTexture* texture;
	StepTimer m_timer;

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

	void CreateDescriptorHeap();
	ID3D12DescriptorHeap* mSrvDescriptorHeap = nullptr;


	vector<SMesh*> m_meshes;
	Light m_lightFromGUI;
	XMVECTOR m_eye;
	XMVECTOR m_at;
	XMVECTOR m_up;

	RayGenConstantBuffer m_rayGenCB;
	// Raytracing scene
	SceneConstantBuffer m_sceneCB[3];
	CubeConstantBuffer m_cubeCB;

	union AlignedSceneConstantBuffer
	{
		SceneConstantBuffer constants;
		uint8_t alignmentPadding[D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT];
	};
	AlignedSceneConstantBuffer* m_mappedConstantData;
	ID3D12Resource*       m_perFrameConstants;

	// Raytracing output
	ID3D12Resource* m_raytracingOutput;
	D3D12_GPU_DESCRIPTOR_HANDLE m_raytracingOutputResourceUAVGpuDescriptor;
	UINT m_raytracingOutputResourceUAVDescriptorHeapIndex;

	// Descriptors
	UINT m_descriptorsAllocated;
	UINT m_descriptorSize;

	// Shader tables
	static const wchar_t* c_hitGroupName;
	static const wchar_t* c_raygenShaderName;
	static const wchar_t* c_closestHitShaderName;
	static const wchar_t* c_missShaderName;
	ID3D12Resource* m_missShaderTable;
	ID3D12Resource* m_hitGroupShaderTable;
	ID3D12Resource* m_rayGenShaderTable;

	// Library subobjects
	static const wchar_t* c_globalRootSignatureName;
	static const wchar_t* c_localRootSignatureName;
	static const wchar_t* c_localRootSignatureAssociationName;
	static const wchar_t* c_shaderConfigName;
	static const wchar_t* c_pipelineConfigName;

	ID3D12StateObject* m_dxrStateObject;
};