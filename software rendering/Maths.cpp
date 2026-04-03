#include "Maths.h"

#include <iostream>

using namespace std;

float Dot(float2 a, float2 b) {
	return a.x * b.x + a.y * b.y;
}

float2 Perpendicular(float2 vec) {
	return float2(vec.y, -vec.x);
}

static bool PointOnRightSideOfLine(float2 a, float2 b, float2 p) {
	float2 ap = p - a;
	float2 abPerp = Perpendicular(b - a);

	//cout << p.x << p.y << '\n' << a.x << a.y << '\n' << ap.x << ap.y << '\n';
	//cout << Dot(ap, abPerp) << '\n';

	return (Dot(ap, abPerp) >= 0);
}

bool PointInTriangle(float2 a, float2 b, float2 c, float2 p) {
	bool sideAB = PointOnRightSideOfLine(a, b, p);
	bool sideBC = PointOnRightSideOfLine(b, c, p);
	bool sideCA = PointOnRightSideOfLine(c, a, p);

	//cout << sideAB << ' ' << sideBC << ' ' << sideCA << ' ';
	//cout << (sideAB && sideBC && sideCA) << ' ';
	//cout << ((sideAB == sideBC) && (sideCA == sideAB)) << '\n';

	//return sideAB && sideBC && sideCA;
	return sideAB == sideBC && sideBC == sideCA;
}