#pragma once
#include "IExecute.h"

using namespace DirectX;

enum RenderMeshShader
{
	None = 0,
	VS,
	MS
};

class CObjectTest3;
class AmpObjectTest;

class Test : public IExecute
{
public:
	virtual BOOL Initialize() override;
	virtual void Ready() override {}
	virtual void Destroy() override;
	virtual void Update() override;
	virtual void PreRender() override {}
	virtual void Render() override;
	virtual void PostRender() override;
	virtual void ResizeScreen() override {}

private:
	DWORD	m_dwInitRefCount;

	CObjectTest3* ModelObject;
	AmpObjectTest* AmpObject;

	RenderMeshShader mode = RenderMeshShader::VS;
protected:

};