#pragma once
#include "Camera.h"

class Freedom : public Camera
{
public:
	Freedom();
	~Freedom();

	void Update() override;
	void Speed(float move, float rotation);

private:
	float move = 20.0f;
	float rotation = 2.0f;
};