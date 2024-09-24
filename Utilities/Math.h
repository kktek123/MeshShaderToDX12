#pragma once
class Math
{
public:
	static const float PI;
	static const float EPSILON;

	static float ToRadian(float degree);
	static float ToDegree(float radian);

	static int Random(int r1, int r2);
	static float Random(float r1, float r2);
	static float SRandom(float r1, float r2);
	
	static SimpleMath::Vector2 RandomVec2(float r1, float r2);
	static SimpleMath::Vector3 RandomVec3(float r1, float r2);
	static SimpleMath::Color RandomColor3();
	static SimpleMath::Color RandomColor4();

	static bool IsZero(float a);
	static bool IsOne(float a);


	static int Clamp(int value, int min, int max);
	static float Clamp(float value, float min, float max);
	static LONG Clamp(LONG value, LONG min, LONG max);
	static UINT Clamp(UINT value, UINT min, UINT max);

	static float Lerp(float value1, float value2, float t);
	static void LerpMatrix(OUT SimpleMath::Matrix& out, const Matrix& m1, const Matrix& m2, float amount);

	static SimpleMath::Quaternion LookAt(const Vector3& origin, const Vector3& target, const Vector3& up);
	static float Gaussian(float val, UINT blurCount);

	static void MatrixDecompose(Matrix& m, OUT Vector3& S, OUT Vector3& R, OUT Vector3& T);
};