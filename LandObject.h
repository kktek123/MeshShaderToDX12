#pragma once
#include "Object.h"
class CTexture;

class CLandObject :public CObject
{
public:
	CLandObject() {};
	~CLandObject() {};
	virtual void Init() override;
	virtual void Update() override;
	virtual void Render() override;;
	virtual void ProcessRender(ID3D12GraphicsCommandList* pCommandList, DWORD ThreadIndex) override;
	virtual void Destroy()override;
	void TextureMaterial(wstring file);
	void SetTrasform(Vector3 vec);
	void Mesh();
private:
	void	Cleanup();

	BOOL	InitRootSinagture();
	BOOL	InitPipelineState();

	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12PipelineState* m_pPipelineState = nullptr;
	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* textureUploadHeap = nullptr;
	DWORD	m_dwInitRefCount = 0;

	// vertex data
	ID3D12Resource* m_pVertexBuffer = nullptr;
	ID3D12Resource* m_pVertexUploadBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};

	// index data
	ID3D12Resource* m_pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};

	//vector<TextureDesc> descs;

	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	int count = 0;

public:
	void BaseMap(wstring file);
	void HeightMap(wstring file);
	void LayerMap(wstring file, wstring alphaFile);
	void AlphaMap(wstring alpha);
	void LayerMap1(wstring layer);
	void LayerMap2(wstring layer);
	void LayerMap3(wstring layer);

	void NormalMap(wstring file);
	void SpecularMap(wstring file);
	void DetailMap(wstring file);
	void SnowMap(wstring file);
	void DistanceMap(wstring file);

public:
	float GetHeight(Vector3& position);
	float GetHeightPick(Vector3& position);
	Vector3 GetPickedPosition();
	void RaiseHeight(Vector3& position, UINT type, UINT range);

	void CreateTestTexture(wstring file);
	void LayerBrush(Vector3& position, UINT range, UINT type);
	void DrawTextureBrush(Vector3& position, UINT range, UINT type);

	//void SetTransform(Transform* stransform);

public:
	void SaveXmlfile(wstring file);
	void Loadfile(wstring file);
	void OpenTexture(wstring filename);

private:
	void CreateVertexData();
	void CreateIndexData();
	void CreateNormalData();


	UINT vertexCount = 0;
	UINT indexCount = 0;
private:
	struct BrushDesc
	{
		Color color = Color(0, 1, 0, 1);
		Vector3 Location;
		UINT Type = 2;
		UINT Range = 30;
		float Padding[3];
	} brushDesc;

	struct LineDesc
	{
		Color color = Color(1, 1, 1, 1);
		UINT Visible = 0;
		float Thickness = 0.01f;
		float Size = 5.0f;
		float Padding;
	} lineDesc;

	Color LayerColor = Color(0, 0, 0, 1);

private:
	CTexture* heightMap;

	//vector<Keyframe> keyframes;
	vector<string> layerMaps;
	vector<Color> alpha_pixel_color;

	string baseName;
	string layer1Name;
	string layer2Name;
	string layer3Name;
	UINT layerNum;

	UINT width;
	UINT height;
	bool bpress = false;
	bool time = false;

	UINT PickIndex = 0;
	vector<bool> isPicked;

	std::vector<Vertex> vertices;
	//vector<SkinnedVertex> skinnedVertices;
	vector<uint32_t> indices;

	CTexture* baseMap = NULL;

	CTexture* layerMap = NULL;

	CTexture* layerMap1 = NULL;

	CTexture* layerMap2 = NULL;


	CTexture* alphaMap = NULL;

	CTexture* normalMap = NULL;

	CTexture* specularMap = NULL;

	CTexture* detailMap = NULL;

	CTexture* snowMap = NULL;

	CTexture* distanceMap = NULL;

	//ConstantBuffer* brushBuffer;
	//ID3DX11EffectConstantBuffer* sBrushBuffer;

	//ConstantBuffer* lineBuffer;
	//ID3DX11EffectConstantBuffer* sLineBuffer;
	//Transform* transform = NULL;
	_declspec(align(256u)) struct SceneConstantBuffer
	{
		XMMATRIX	matWorld = XMMatrixIdentity();
		XMMATRIX	matView = XMMatrixIdentity();
		XMMATRIX	matProj = XMMatrixIdentity();
		//uint32_t   DrawMeshlets;
	};

	ID3D12Resource* m_constantBuffer;
	UINT8* m_cbvDataBegin;
	SceneConstantBuffer m_constantBufferData;
};