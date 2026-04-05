#include "Maths.h"

#include <iostream>

using namespace std;


float2 Perpendicular(float2 vec) {
	return float2(vec.y, -vec.x);
}

float SignedTriangleArea(float2 a, float2 b, float2 c) {
	float2 ac = c - a;
	float2 abPerb = Perpendicular(b - a);
	return float2::Dot(ac, abPerb) / 2;
}

bool PointInTriangle(float2 a, float2 b, float2 c, float2 p, float3& weights) {
	float areaABP = SignedTriangleArea(a, b, p);
	float areaBCP = SignedTriangleArea(b, c, p);
	float areaCAP = SignedTriangleArea(c, a, p);

	bool inTri = areaABP >= 0 && areaBCP >= 0 && areaCAP >= 0;

	float invAreaSum = 1 / (areaABP + areaBCP + areaCAP);
	float weightA = areaBCP * invAreaSum;
	float weightB = areaCAP * invAreaSum;
	float weightc = areaABP * invAreaSum;
	weights = float3(weightA, weightB, weightc);

	return inTri;
}
