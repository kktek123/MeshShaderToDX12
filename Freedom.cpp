#include "pch.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Freedom.h"

Freedom::Freedom()
	: Camera()
{

}

Freedom::~Freedom()
{

}

void Freedom::Update()
{
	if (Mouse::Get()->Press(1) == false) return;

	Vector3 f = Forward();
	Vector3 u = Up();
	Vector3 r = Right();

	//Move
	{
		Vector3 P;
		Position(&P);

		if (Keyboard::Get()->Press('W'))
			P = P + f * move * 0.0001;// *Time::Delta();
		else if (Keyboard::Get()->Press('S'))
			P = P - f * move * 0.0001;// * Time::Delta();

		if (Keyboard::Get()->Press('D'))
			P = P + r * move * 0.0001;// * Time::Delta();
		else if (Keyboard::Get()->Press('A'))
			P = P - r * move * 0.0001;// * Time::Delta();

		if (Keyboard::Get()->Press('E'))
			P = P + u * move * 0.0001;// * Time::Delta();
		else if (Keyboard::Get()->Press('Q'))
			P = P - u * move * 0.0001;// * Time::Delta();

		Position(P);
	}

	//Rotation
	{
		Vector3 R;
		Rotation(&R);

		Vector3 val = Mouse::Get()->GetMoveValue();
		R.x = R.x + val.y * rotation * 0.001;//* Time::Delta();
		R.y = R.y + val.x * rotation * 0.001;//* Time::Delta();
		R.z = 0.0f;

		Rotation(R);
	}
}

void Freedom::Speed(float move, float rotation)
{
	this->move = move;
	this->rotation = rotation;
}
