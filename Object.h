#pragma once
#include "pch.h"

class CObject
{
public:
	CObject() {};
	~CObject() {};
	virtual void Init() {};
	virtual void Update() {};
	virtual void Render() {};
	virtual void ProcessRender(ID3D12GraphicsCommandList* pCommandList,DWORD ThreadIndex) {};
	virtual void UpdateTransform() {};
	virtual void Destroy() {};
	XMFLOAT3 Position;

	XMMATRIX m_matScale = {};
	XMMATRIX m_matRot = {};
	XMMATRIX m_matTrans = {};
	XMMATRIX m_matWorld = {};
protected:

private:
};