#pragma once
#include "IExecute.h"

class D3D12MeshletRender;
class Main : public IExecute
{
public:
	virtual BOOL Initialize() override;
	virtual void Ready() override;
	virtual void Destroy() override;
	virtual void Update() override;
	virtual void PreRender() override;
	virtual void Render() override;
	virtual void PostRender() override;
	virtual void ResizeScreen() override;

private:
	void Push(IExecute* execute);

private:
	vector<IExecute*> executes;

};