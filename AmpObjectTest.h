#pragma once
#include "Object.h"
#include "Shared.h"
#include "d3dUtil.h"
//#include <DirectXMesh.h>

class CTexture;

class AmpObjectTest : public CObject
{
public:
	AmpObjectTest() {};
	~AmpObjectTest() {};
	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;
	virtual void ProcessRender(ID3D12GraphicsCommandList* pCommandList, DWORD ThreadIndex) {};
	virtual void Destroy()override;
	void SetTrasform(Vector3 vec);
	void Mesh();
protected:

private:

	void	Cleanup();

	BOOL	InitConstantBuffer();
	BOOL	InitRootSinagtureAndPipelineState();
	HRESULT ReadDataFromFile(LPCWSTR filename, std::byte** data, UINT* size);


	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;
	DWORD	m_dwInitRefCount = 0;

	struct SMesh {

		ID3D12Resource* vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		ID3D12Resource* indexBuffer;
		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		ID3D12Resource* meshletBuffer;
		ID3D12Resource* uniqueVertexIBBuffer;
		ID3D12Resource* primitiveIndicesBuffer;		
		ID3D12Resource* meshInfoBuffer;
		ID3D12Resource* CullDataBuffer;
		size_t MeshletOffset = 0;
		size_t MeshletCount = 0;


		CTexture* texture;
		UINT m_indexCount = 0;
	};


	_declspec(align(256u)) struct SceneConstantBuffer
	{
		XMFLOAT4X4 World;
		XMFLOAT4X4 WorldView;
		XMFLOAT4X4 WorldViewProj;
		//XMFLOAT4X4 TexTransform;
		uint32_t   DrawMeshlets;
	};


	ID3D12Resource* m_constantBuffer;
	UINT8* m_cbvDataBegin;
	Constants m_constantBufferData;

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

	struct MeshInfo
	{
		uint IndexSize;
		uint MeshletCount;

		uint LastMeshletVertCount;
		uint LastMeshletPrimCount;
	};

	inline uint32_t DivRoundUp(uint32_t num, uint32_t den) { return (num + den - 1) / den; }

	vector<SceneObject>           m_objects;
};