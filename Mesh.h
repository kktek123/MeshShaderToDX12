//#pragma once
//#include "VertexData.h"
//class ResourceManager;
//
//class BasicMesh
//{
//public:
//	//typedef VertexTextureNormalTangent MeshVertex;
//	static const UINT DESCRIPTOR_COUNT_PER_OBJ = 1;			// | Constant Buffer
//	static const UINT DESCRIPTOR_COUNT_PER_TRI_GROUP = 1;	// | SRV(tex)
//	static const UINT MAX_TRI_GROUP_COUNT_PER_OBJ = 8;
//	static const UINT MAX_DESCRIPTOR_COUNT_FOR_DRAW = DESCRIPTOR_COUNT_PER_OBJ + (MAX_TRI_GROUP_COUNT_PER_OBJ * DESCRIPTOR_COUNT_PER_TRI_GROUP);
//
//public:
//
//	void Initialize(ResourceManager* resourceManager,
//		const std::string& basePath, const std::string& filename);
//	void Initialize(ResourceManager* resourceManager, 
//		const std::vector<MeshData>& meshes);
//
//	void UpdateConstantBuffers(CSimpleConstantBufferPool* ConstantBufferPool);
//
//	void UpdateModelWorld(const Matrix& modelToWorldRow);
//
//
//	//void SetShader(Shader* shader);
//	//void Pass(UINT val) { pass = val; }
//
//	void Render();
//
//protected:
//	//virtual void Create() = 0;
//private:
//	BOOL	InitCommonResources();
//	void	CleanupSharedResources();
//
//	BOOL	InitRootSinagture();
//	BOOL	InitPipelineState();
//
//	//void	DeleteTriGroup(INDEXED_TRI_GROUP* pTriGroup);
//	void	Cleanup();
//
//	std::vector<shared_ptr<Mesh>> m_meshes; // Mipmaps 생성을 위해 임시로
//
//	ID3D12RootSignature* m_pRootSignature = nullptr;
//	ID3D12PipelineState* m_pPipelineState = nullptr;
//	ID3D12Resource* pTexResource = nullptr;
//	ID3D12Resource* textureUploadHeap = nullptr;
//	DWORD	m_dwInitRefCount = 0;
//
//	// vertex data
//	ID3D12Resource* m_pVertexBuffer = nullptr;
//	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};
//
//	// index data
//	ID3D12Resource* m_pIndexBuffer = nullptr;
//	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};
//	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
//
//	Matrix m_matWorld;
//
//};