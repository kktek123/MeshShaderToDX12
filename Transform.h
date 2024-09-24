#pragma once

class Transform
{
public:
	Transform();
	~Transform();

	void Set(Transform* transform);

	//void SetShader(Shader* shader);

	void Position(float x, float y, float z);
	void Position(Vector3& vec);
	void Position(Vector3* vec);
	Vector3 GetPosition() { return position; }

	void Scale(float x, float y, float z);
	void Scale(Vector3& vec);
	void Scale(Vector3* vec);
	Vector3 GetScale() { return scale; }

	void Rotation(float x, float y, float z);
	void Rotation(Vector3& vec);
	void Rotation(Vector3* vec);
	Vector3 GetRotation() { return rotation; }

	void RotationDegree(float x, float y, float z);
	void RotationDegree(Vector3& vec);
	void RotationDegree(Vector3* vec);
	Vector3 RotationDegree();

	Vector3 Forward();
	Vector3 Up();
	Vector3 Right();

	void World(Matrix& matrix);
	Matrix& World() { return bufferDesc.World; }

private:
	void UpdateWorld();

public:
	void Update();
	void Render();

private:
	struct BufferDesc
	{
		Matrix World;
	} bufferDesc;

private:
	//Shader* shader;



	Vector3 position;
	Vector3 scale;
	Vector3 rotation;
};