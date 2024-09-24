#pragma once

class BinaryWriter
{
public:
	BinaryWriter();
	~BinaryWriter();

	void Open(wstring filePath, UINT openOption = CREATE_ALWAYS);
	void Close();

	void Bool(bool data);
	void Word(WORD data);
	void Int(int data);
	void UInt(UINT data);
	void Float(float data);
	void Double(double data);

	void Vector2(const SimpleMath::Vector2& data);
	void Vector3(const SimpleMath::Vector3& data);
	void Vector4(const SimpleMath::Vector4& data);
	void Color3f(const SimpleMath::Color& data);
	void Color4f(const SimpleMath::Color& data);
	void Matrix(const SimpleMath::Matrix& data);

	void String(const string& data);
	void Byte(void* data, UINT dataSize);

protected:
	HANDLE fileHandle;
	DWORD size;
};

//////////////////////////////////////////////////////////////////////////

class BinaryReader
{
public:
	BinaryReader();
	~BinaryReader();

	void Open(wstring filePath);
	void Close();

	bool Bool();
	WORD Word();
	int Int();
	UINT UInt();
	float Float();
	double Double();

	SimpleMath::Vector2 Vector2();
	SimpleMath::Vector3 Vector3();
	SimpleMath::Vector4 Vector4();
	SimpleMath::Color Color3f();
	SimpleMath::Color Color4f();
	SimpleMath::Matrix Matrix();

	string String();
	void Byte(void** data, UINT dataSize);

protected:
	HANDLE fileHandle;
	DWORD size;
};