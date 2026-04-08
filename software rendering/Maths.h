#pragma once

#include <math.h>
#include <algorithm>

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

	float2& operator+=(const float2 &n) {
		x += n.x;
		y += n.y;
		return *this;
	}

	float2& operator*=(const float &n) {
		x *= n;
		y *= n;
		return *this;
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

	static float2 Saturate(float2 texCoord) {
		return float2(std::clamp(texCoord.x, 0.0f, 1.0f), std::clamp(texCoord.y, 0.0f, 1.0f));
	}

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

	float3& operator+=(const float3& n) {
		x += n.x;
		y += n.y;
		z += n.z;
		return *this;
	}

	float3& operator-=(const float3& n) {
		x -= n.x;
		y -= n.y;
		z -= n.z;
		return *this;
	}

	float3& operator*=(const float& n) {
		x *= n;
		y *= n;
		z *= n;
		return *this;
	}

	float3 operator-(const float3& n) {
		return float3(x - n.x, y - n.y, z - n.z);
	}

	float3 operator*(const float& n) {
		return float3(x * n, y * n, z * n);
	}

	float3& operator/=(float s) {
		float inv = 1.0f / s;
		x *= inv;
		y *= inv;
		z *= inv;
		return *this;
	}

	float3 operator/(const float& n) {
		return float3(x / n, y / n, z / n);
	}

	operator float2() const {
		return float2(x, y);
	}

	static float Dot(float3 a, float3 b) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	};

	static float3 Normalize(float3 v) {
		if (v.x == 0 && v.y == 0 && v.z == 0) return float3(0, 0, 0);
		return v / sqrtf(Dot(v, v));
	}
};

inline float3 operator/(const float3& v1, const float3& v2) {
	return { v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
}

inline float3 operator/(float s, const float3& v) {
	return { s / v.x, s / v.y, s / v.z };
}

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
	float3 Scale;

	Transform(float yaw, float pitch, float3 positon, float3 scale): Yaw(yaw), Pitch(pitch), 
		Position(positon), Scale(scale) {}
	Transform(float3 position) : Transform(0.0f, 0.0f, position, float3(1.0, 1.0, 1.0)){}
	Transform(float3 position, float3 scale) : Transform(0.0f, 0.0f, position, scale) {}

	float3 ToWorldPoint(float3 p) {
		auto basic = GetBasisVectors();
		basic.ihat *= Scale.x;
		basic.jhat *= Scale.y;
		basic.khat *= Scale.z;
		return TransformVector(basic.ihat, basic.jhat, basic.khat, p) + Position;
	}

	float3 ToLocalPoint(float3 worldPoint) {
		auto basic = GetInverseBasisVectors();
		auto local = TransformVector(basic.ihat, basic.jhat, basic.khat, worldPoint - Position);
		local.x /= Scale.x;
		local.y /= Scale.y;
		local.z /= Scale.z;
		return local;
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

	IJK GetInverseBasisVectors() const {

		auto hats = GetBasisVectors();

		return { float3(hats.ihat.x, hats.jhat.x, hats.khat.x), float3(hats.ihat.y, hats.jhat.y, hats.khat.y), float3(hats.ihat.z, hats.jhat.z, hats.khat.z) };
	}

	static float3 TransformVector(float3 ihat, float3 jhat, float3 khat, float3 v) {
		return ihat * v.x + jhat * v.y + khat * v.z;
	}
};