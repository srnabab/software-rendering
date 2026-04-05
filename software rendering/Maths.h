#pragma once

#include <math.h>

class float2
{
	public:
		union {
			struct { float x, y; };
			struct { float u, v; };
			float data[2];
		};
	float2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

	float2 operator+(const float2 &n) {
		return float2(x + n.x, y + n.y);
	}

	float2 operator-(const float2& n) {
		return float2(x - n.x, y - n.y);
	}

	float2 operator/(const float& n) {
		return float2(x / n, y / n);
	}

	float2 operator*(const float& n) {
		return float2(x * n, y * n);
	}

	static float Dot(float2 a, float2 b) {
		return a.x * b.x + a.y * b.y;
	};

};


class float3
{
public:
	union {
		struct { float x, y, z; };
		struct { float r, g, b; };
		float data[3];
	};

	float3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

	float3 operator+(const float3& n) {
		return float3(x + n.x, y + n.y, z + n.z);
	}

	float3 operator*(const float& n) {
		return float3(x * n, y * n, z * n);
	}

	operator float2() const {
		return float2(x, y);
	}

	static float Dot(float3 a, float3 b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	};
};

float2 Perpendicular(float2 vec);

float SignedTriangleArea(float2 a, float2 b, float2 c);
bool PointInTriangle(float2 a, float2 b, float2 c, float2 p, float3& out);


typedef struct _IJK {
	float3	ihat;
	float3 jhat;
	float3 khat;
}IJK;

class Transform {
public:
	float Yaw;
	float Pitch;
	float3 Position;

	Transform(float yaw, float pitch, float3 positon): Yaw(yaw), Pitch(pitch), Position(positon) {}
	Transform(float3 position) : Transform(0.0f, 0.0f, position){}

	float3 ToWorldPoint(float3 p) {
		auto basic = GetBasisVectors();
		return TransformVector(basic.ihat, basic.jhat, basic.khat, p) + Position;
	}

	IJK GetBasisVectors() const {
		float cosYaw = cosf(Yaw);
		float sinYaw = sinf(Yaw);

		float cosPitch = cosf(Pitch);
		float sinPitch = sinf(Pitch);

		float3 ihat_yaw(cosYaw, 0.0f, sinYaw);
		float3 jhat_yaw(0.0f, 1.0f, 0.0f);
		float3 khat_yaw(-sinYaw, 0.0f, cosYaw);

		float3 ihat_pitch(1.0f, 0.0f, 0.0f);
		float3 jhat_pitch(0, cosPitch, -sinPitch);
		float3 khat_pitch(0, sinPitch, cosPitch);

		float3 ihat = TransformVector(ihat_yaw, jhat_yaw, khat_yaw, ihat_pitch);
		float3 jhat = TransformVector(ihat_yaw, jhat_yaw, khat_yaw, jhat_pitch);
		float3 khat = TransformVector(ihat_yaw, jhat_yaw, khat_yaw, khat_pitch);

		return { ihat, jhat, khat };
	}

	static float3 TransformVector(float3 ihat, float3 jhat, float3 khat, float3 v) {
		return ihat * v.x + jhat * v.y + khat * v.z;
	}
};