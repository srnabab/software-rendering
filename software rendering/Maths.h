#pragma once
class float3
{
	public:
		union {
			struct { float x, y, z; };
			struct { float r, g, b; };
			float data[3];
		};

	float3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}
};

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

};


float Dot(float2 a, float2 b);

float2 Perpendicular(float2 vec);

bool PointInTriangle(float2 a, float2 b, float2 c, float2 p);