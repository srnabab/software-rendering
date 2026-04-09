#include <iostream>
using namespace std;

#include <vector>
#include <fstream>
#include <string>
#include <cstdint>
#include <random>
#include "MiniFB.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <numbers>

#include "Window.h"
#include "shaders.h"

constexpr auto width = 800;
constexpr auto height = 600;

std::vector<float2> points;
std::vector<float2> velocities;
std::vector<float3> triangleCols;
std::vector<float3> image;
std::vector<uint32_t> buffers;

uint32_t frame = 0;

struct ObjVertex {
	std::vector<float3> position;
	std::vector<float2> texCoord;
	std::vector<float3> normal;
};

static ObjVertex LoadObjFile(const std::string& filename) {
	std::vector<float3> allPoints;
	std::vector<float3> normalPoints;
	std::vector<float2> texCoordPoints;

	std::vector<float3> trianglePoints;
	std::vector<float2> texCoord;
	std::vector<float3> normal;

	string line;

	ifstream file(filename);
	if (file.is_open()) {
		while (std::getline(file, line)) {
			if (line.back() == '\r')
				line.pop_back();

			if (line.substr(0, 2) == "v ") {
				auto firstSpace = line.substr(2).find(' ') + 2;
				auto secondSpace = line.substr(firstSpace + 1).find(' ') + firstSpace + 1;

				allPoints.push_back(float3(
					stof(line.substr(2, firstSpace)),
					stof(line.substr(firstSpace + 1, secondSpace - firstSpace)),
					stof(line.substr(secondSpace + 1))
				));

				//cout << "Loaded vertex: " << allPoints.back().x << ' ' << allPoints.back().y << ' ' << allPoints.back().z << '\n';
			}
			else if (line.substr(0, 3) == "vn ") {
				auto firstSpace = line.substr(3).find(' ') + 3;
				auto secondSpace = line.substr(firstSpace + 1).find(' ') + firstSpace + 1;

				normalPoints.push_back(float3(
					stof(line.substr(3, firstSpace)),
					stof(line.substr(firstSpace + 1, secondSpace - firstSpace)),
					stof(line.substr(secondSpace + 1))
				));
			}
			else if (line.substr(0, 3) == "vt ") {
				auto firstSpace = line.substr(3).find(' ') + 3;

				texCoordPoints.push_back(float2(
					stof(line.substr(3, firstSpace)),
					stof(line.substr(firstSpace + 1))
				));
			}
			else if (line.substr(0, 2) == "f ") {
				std::vector<string> faceIndexGroups;
				auto current = 2;
				while (current <= line.size()) {
					auto nextSpace = line.substr(current).find(' ') + current;

					if (nextSpace == current - 1) {
						faceIndexGroups.push_back(line.substr(current));
						break;
					}

					faceIndexGroups.push_back(line.substr(current, nextSpace - current));
					current = nextSpace + 1;
				}

				for (size_t i = 0; i < faceIndexGroups.size(); i++)
				{
					auto currentSlash = 0;
					int times = 0;

					if (faceIndexGroups[i].find("//") != std::string::npos) {
						int indexs[2];
						indexs[0] = stoi(faceIndexGroups[i].substr(0, faceIndexGroups[i].find("/")));
						indexs[1] = stoi(faceIndexGroups[i].substr(faceIndexGroups[i].find("/") + 2));

						for (size_t j = 0; j < 2; j++)
						{
							if (indexs[j] < 0) {

								if (j == 0) {

									if (i >= 3) {
										trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - 2));
									}
									//cout << pointIndex << '\n';
									trianglePoints.push_back(allPoints[allPoints.size() + indexs[j]]);
								}
								else if (j == 1) {

									if (i >= 3) {
										normal.push_back(normal.at(normal.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										normal.push_back(normal.at(normal.size() - 2));
									}
									normal.push_back(normalPoints[normalPoints.size() + indexs[j]]);
								}
							}
							else {
								indexs[j] -= 1;

								if (j == 0) {

									if (i >= 3) {
										trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - 2));
									}
									trianglePoints.push_back(allPoints[indexs[j]]);
								}
								else if (j == 1) {

									if (i >= 3) {
										normal.push_back(normal.at(normal.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										normal.push_back(normal.at(normal.size() - 2));
									}
									normal.push_back(normalPoints[indexs[j]]);
								}
							}
						}
					}
					else {


						while (currentSlash <= faceIndexGroups[i].size()) {
							auto nextSlash = faceIndexGroups[i].substr(currentSlash).find('/') + currentSlash;


							if (times == 3) break;

							int pointIndex = stoi(faceIndexGroups[i].substr(currentSlash, nextSlash - currentSlash));
							if (pointIndex < 0) {

								if (times == 0) {

									if (i >= 3) {
										trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - 2));
									}
									//cout << pointIndex << '\n';
									trianglePoints.push_back(allPoints[allPoints.size() + pointIndex]);
								}
								else if (times == 1) {

									if (i >= 3) {
										texCoord.push_back(texCoord.at(texCoord.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										texCoord.push_back(texCoord.at(texCoord.size() - 2));
									}
									texCoord.push_back(texCoordPoints[allPoints.size() + pointIndex]);
								}
								else if (times == 2) {

									if (i >= 3) {
										normal.push_back(normal.at(normal.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										normal.push_back(normal.at(normal.size() - 2));
									}
									normal.push_back(normalPoints[allPoints.size() + pointIndex]);
								}
							}
							else {
								pointIndex -= 1;

								if (times == 0) {

									if (i >= 3) {
										trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - 2));
									}
									trianglePoints.push_back(allPoints[pointIndex]);
								}
								else if (times == 1) {

									if (i >= 3) {
										texCoord.push_back(texCoord.at(texCoord.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										texCoord.push_back(texCoord.at(texCoord.size() - 2));
									}
									texCoord.push_back(texCoordPoints[pointIndex]);
								}
								else if (times == 2) {

									if (i >= 3) {
										normal.push_back(normal.at(normal.size() - (3 * i - 6)));
									}
									if (i >= 3) {
										normal.push_back(normal.at(normal.size() - 2));
									}
									normal.push_back(normalPoints[pointIndex]);
								}
							}

							times++;
							currentSlash = nextSlash + 1;
							//break;
						}
					}
				}
			}
		}
	}

	//for (size_t i = 0; i < trianglePoints.size(); i++)
	//{
		//cout << "Triangle vertex: " << i << ' ' << trianglePoints[i].x << ' ' << trianglePoints[i].y << ' ' << trianglePoints[i].z << '\n';
	//}
	//cout << trianglePoints.size() << '\n';

	return ObjVertex{ .position = trianglePoints, .texCoord = texCoord, .normal = normal };
}

typedef struct _RasterizerPoint {
	float Depth;
	float2 ScreenPos;
	float2 TexCoords;
	float3 Normals;
}RasterizerPoint;


template <typename T>
class Model {
public:
	std::vector<float3>Points;
	std::vector<float2>TexCoords;
	std::vector<float3> Normals;
	std::vector<RasterizerPoint> RasterizerPoints;
	T shader;

	Model(const std::vector<float3>& points, const T& shader, const std::vector<float2>& texCoords, const std::vector<float3>& normals) :
		Points(points), shader(shader), TexCoords(texCoords), Normals(normals) {}

	static Model<T> LoadFromObj(const std::string& filename, T& shader) {
		auto vertices = LoadObjFile(filename + ".obj");

		return Model<T>(vertices.position, shader, vertices.texCoord, vertices.normal);
	}
};

class RenderTarget {
public:
	uint32_t Width;
	uint32_t Height;
	float2 Size = float2(Width, Height);

	std::vector<uint32_t> Buffers;
	std::vector<float> DepthBuffer;

	RenderTarget(uint32_t width, uint32_t height) : Width(width), Height(height), Buffers(width* height, 0), DepthBuffer(width* height, 10000000000.0f) {}

	void Clear() {
		memset(Buffers.data(), 0, Buffers.size() * sizeof(uint32_t));
		std::fill(DepthBuffer.begin(), DepthBuffer.end(), 10000000000.0f);
	}
};


const std::vector<float3> DISTINCT_COLORS = {
	{255, 0, 0},
	{0, 255, 0},
	{0, 0, 255},
	{255, 255, 0},
	{255, 0, 255},
	{0, 255, 255},
	{255, 128, 0},
	{128, 0, 255},
	{0, 128, 128},
	{255, 153, 204},
	{128, 255, 0},
	{0, 0, 128},
	{153, 0, 0},
	{204, 255, 153},
	{102, 51, 0},
	{51, 102, 204},
	{255, 204, 153},
	{0, 102, 0},
	{102, 102, 102},
	{153, 153, 255},
	{255, 102, 102},
	{204, 204, 0},
	{102, 255, 204}
};

static float3 GenRandomColor() {
	static int id = 0;

	id = (id + 3) % DISTINCT_COLORS.size();

	return DISTINCT_COLORS[id];
}

static float degToRad(float degrees) {
	return degrees * (std::numbers::pi / 180.0f);
}

static float radToDeg(float radians) {
	return radians * (180.0f / std::numbers::pi);
}

class Camera {
public:
	float fov;
	Transform transform;

	Camera(float fov, Transform transform) : fov(fov), transform(transform) {}
	Camera(Transform transform) : Camera(degToRad(60.0f), transform) {}
};

static void CreateTestImage() {
	const int triangleCount = 250;
	points = std::vector<float2>(triangleCount * 3);
	velocities = std::vector<float2>(points.size());
	triangleCols = std::vector<float3>(triangleCount);
	image = std::vector<float3>(width * height, float3(0.0f, 0.0f, 0.0f));

	float2 halfSize = float2(width, height) / 2.0f;

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_real_distribution<float> dis1(0.0f, static_cast<float>(width));
	std::uniform_real_distribution<float> dis2(0.0f, static_cast<float>(height));
	std::uniform_int_distribution<int> disCol(0, 255);

	for (size_t i = 0; i < points.size(); i++)
	{
		points[i] = halfSize + (float2(dis1(gen), dis2(gen)) - halfSize) * 0.3f;
	}

	for (size_t i = 0; i < velocities.size(); i+=3)
	{
		float2 velocity = (float2(dis1(gen), dis2(gen)) - halfSize) * 0.2f;
		velocities[i + 0] = velocity;
		velocities[i + 1] = velocity;
		velocities[i + 2] = velocity;

		triangleCols[i / 3] = float3(static_cast<uint8_t>(disCol(gen)), static_cast<uint8_t>(disCol(gen)), static_cast<uint8_t>(disCol(gen)));
		
	}
}

float3 VertexToScreen(float3 vertex, Transform transform, Camera cam, float2 numPixels) {
	float3 vertex_world = transform.ToWorldPoint(vertex); 
	float3 vertex_view = cam.transform.ToLocalPoint(vertex_world);
	float depth = vertex_view.z;

	float screenHeight_world = std::tanf(cam.fov / 2) * 2;
	float pixelsPerWorldUnit = numPixels.y / screenHeight_world / vertex_view.z;

	float2 pixelOffset = float2(vertex_view.x, vertex_view.y) * pixelsPerWorldUnit;
	float2 vertex_screen = numPixels / 2.0f + pixelOffset;

	return float3(vertex_screen.x, vertex_screen.y, depth);
}

float3 VertexToView(float3 vertex, Transform transform, Camera cam) {
	float3 vertex_world = transform.ToWorldPoint(vertex);
	float3 vertex_view = cam.transform.ToLocalPoint(vertex_world);

	return vertex_view;
}

float3 ViewToScreen(float3 vertex_view, Camera cam, float2 numPixels) {
	float screenHeight_world = std::tanf(cam.fov / 2) * 2;
	float pixelsPerWorldUnit = numPixels.y / screenHeight_world / vertex_view.z;

	float2 pixelOffset = float2(vertex_view.x, vertex_view.y) * pixelsPerWorldUnit;
	float2 vertex_screen = numPixels / 2.0f + pixelOffset;

	return float3(vertex_screen.x, vertex_screen.y, vertex_view.z);
}

inline void AtomicDepthTestAndWrite(std::vector<float>& depthBuffer, std::vector<uint32_t>& colorBuffer, int idx, float newDepth, uint32_t newColor) {
	union { float f; uint32_t i; } newVal;
	newVal.f = newDepth;

	std::atomic_ref<float> target{ depthBuffer[idx] };

	float oldDepth = target.load(std::memory_order_relaxed);
	union { uint32_t i; float f; } oldVal;

	while (true) {
		oldVal.i = oldDepth;
		if (newDepth >= oldVal.f) break;

		if (target.compare_exchange_weak(oldDepth, newVal.i, std::memory_order_relaxed)) {
			colorBuffer[idx] = newColor;
			break;
		}
	}
}

template <typename T>
void AddRasterizerPoint(Model<T>& model, float3 viewPoint, int vertIndexA, int vertIndexB, float t, Camera cam, float2 numPixels) {
	auto point = RasterizerPoint{
		.Depth = viewPoint.z,
		.ScreenPos = ViewToScreen(viewPoint, cam, numPixels),
		.TexCoords = float2(),
		.Normals = float3::Lerp(model.Normals[vertIndexA], model.Normals[vertIndexB], t)
	};

	if (model.TexCoords.size() > 0) {
		point.TexCoords = float2::Lerp(model.TexCoords[vertIndexA], model.TexCoords[vertIndexB], t);
	}

	model.RasterizerPoints.push_back(point);
}

template <typename T>
void AddRasterizerPoint(Model<T>& model, float3 viewPoint, int vertIndex, Camera cam, float2 numPixels) {
	auto point = RasterizerPoint{
	.Depth = viewPoint.z,
	.ScreenPos = ViewToScreen(viewPoint, cam, numPixels),
	.TexCoords = float2(),
	.Normals = model.Normals[vertIndex]
	};

	if (model.TexCoords.size() > 0) {
		point.TexCoords = model.TexCoords[vertIndex];
	}

	model.RasterizerPoints.push_back(point);
}


template <typename T>
static void ProcessModel(Model<T>& model, Transform& transform, Camera cam, float2 numPixels) {
	float3 viewPoints[3];
	model.RasterizerPoints.clear();

	for (int i = 0; i < model.Points.size(); i += 3)
	{
		viewPoints[0] = VertexToView(model.Points[i + 0], transform, cam);
		viewPoints[1] = VertexToView(model.Points[i + 1], transform, cam);
		viewPoints[2] = VertexToView(model.Points[i + 2], transform, cam);

		const float nearClipDst = 0.01f;
		bool clip0 = viewPoints[0].z <= nearClipDst;
		bool clip1 = viewPoints[1].z <= nearClipDst;
		bool clip2 = viewPoints[2].z <= nearClipDst;
		int clipCount = static_cast<int>(clip0) + static_cast<int>(clip1) + static_cast<int>(clip2);

		switch (clipCount) {
		case 0:
		
			AddRasterizerPoint(model, viewPoints[0], i + 0, cam, numPixels);
			AddRasterizerPoint(model, viewPoints[1], i + 1, cam, numPixels);
			AddRasterizerPoint(model, viewPoints[2], i + 2, cam, numPixels);
			break;

		case 1:
		{
			int indexClip = clip0 ? 0 : (clip1 ? 1 : 2);
			int indexNext = (indexClip + 1) % 3;
			int indexPrev = (indexClip - 1 + 3) % 3;
			float3 pointClipped = viewPoints[indexClip];
			float3 pointA = viewPoints[indexNext];
			float3 pointB = viewPoints[indexPrev];

			float fracA = (nearClipDst - pointClipped.z) / (pointA.z - pointClipped.z);
			float fracB = (nearClipDst - pointClipped.z) / (pointB.z - pointClipped.z);

			float3 clipPointAlongEageA = float3::Lerp(pointClipped, pointA, fracA);
			float3 clipPointAlongEageB = float3::Lerp(pointClipped, pointB, fracB);

			AddRasterizerPoint(model, clipPointAlongEageB, i + indexClip, i + indexPrev, fracB, cam, numPixels);
			AddRasterizerPoint(model, clipPointAlongEageA, i + indexClip, i + indexNext, fracA, cam, numPixels);
			AddRasterizerPoint(model, pointB, i + indexPrev, cam, numPixels);

			AddRasterizerPoint(model, clipPointAlongEageA, i + indexClip, i + indexNext, fracA, cam, numPixels);
			AddRasterizerPoint(model, pointA, i + indexNext, cam, numPixels);
			AddRasterizerPoint(model, pointB, i + indexPrev, cam, numPixels);
			break;
		}

		case 2:
		{
			int indexNonClip = !clip0 ? 0 : (!clip1 ? 1 : 2);
			int indexNext = (indexNonClip + 1) % 3;
			int indexPrev = (indexNonClip - 1 + 3) % 3;

			float3 pointNonClipped = viewPoints[indexNonClip];
			float3 pointA = viewPoints[indexNext];
			float3 pointB = viewPoints[indexPrev];

			float fracA = (nearClipDst - pointNonClipped.z) / (pointA.z - pointNonClipped.z);
			float fracB = (nearClipDst - pointNonClipped.z) / (pointB.z - pointNonClipped.z);

			float3 clipPointAlongEageA = float3::Lerp(pointNonClipped, pointA, fracA);
			float3 clipPointAlongEageB = float3::Lerp(pointNonClipped, pointB, fracB);

			AddRasterizerPoint(model, clipPointAlongEageB, i + indexNonClip, i + indexPrev, fracB, cam, numPixels);
			AddRasterizerPoint(model, pointNonClipped, i + indexNonClip, cam, numPixels);
			AddRasterizerPoint(model, clipPointAlongEageA, i + indexNonClip, i + indexNext, fracA, cam, numPixels);
			break;
		}
		}
	}
}

template <typename T>
static void Render(Model<T>& model, Transform& transform, RenderTarget& target, Camera cam) {
	ProcessModel(model, transform, cam, target.Size);

#pragma omp parallel for
	for (int i = 0; i < model.RasterizerPoints.size(); i += 3)
	{
		//auto a = VertexToScreen(model.RasterizerPoints[i + 0], transform, cam, target.Size);
		//auto b = VertexToScreen(model.RasterizerPoints[i + 1], transform, cam, target.Size);
		//auto c = VertexToScreen(model.RasterizerPoints[i + 2], transform, cam, target.Size);
		auto a = model.RasterizerPoints[i + 0].ScreenPos;
		auto b = model.RasterizerPoints[i + 1].ScreenPos;
		auto c = model.RasterizerPoints[i + 2].ScreenPos;

		//if (a.z <= 0 || b.z <= 0 || c.z <= 0) continue;

		float minX = std::min(std::min(a.x, b.x), c.x);
		float minY = std::min(std::min(a.y, b.y), c.y);
		float maxX = std::max(std::max(a.x, b.x), c.x);
		float maxY = std::max(std::max(a.y, b.y), c.y);

		//Pixel block covering the triangle bounds

		int blockStartX = std::clamp(static_cast<int>(minX), 0, static_cast<int>(target.Width - 1));
		int blockStartY = std::clamp(static_cast<int>(minY), 0, static_cast<int>(target.Height - 1));
		int blockEndX = std::clamp(static_cast<int>(std::ceil(maxX)), 0, static_cast<int>(target.Width - 1));
		int blockEndY = std::clamp(static_cast<int>(std::ceil(maxY)), 0, static_cast<int>(target.Height - 1));

		for (int y = blockStartY; y < blockEndY; y++)
		{
			for (int x = blockStartX; x < blockEndX; x++)
			{
				float2 p = float2(static_cast<float>(x), static_cast<float>(y));
				float3 weights;

				if (PointInTriangle((float2)a, (float2)b, (float2)c, p, weights)) {

					float3 depths = float3(model.RasterizerPoints[i + 0].Depth, model.RasterizerPoints[i + 1].Depth,
						model.RasterizerPoints[i + 2].Depth);
					float depth = 1 / float3::Dot(1 / depths, weights);

					if (depth > target.DepthBuffer[x + y * target.Width]) continue;

					float2 texCoord = float2();
					texCoord += model.RasterizerPoints[i + 0].TexCoords / depths.data[0] * weights.x;
					texCoord += model.RasterizerPoints[i + 1].TexCoords / depths.data[1] * weights.y;
					texCoord += model.RasterizerPoints[i + 2].TexCoords / depths.data[2] * weights.z;
					texCoord *= depth;

					float3 normal = float3();
					normal += model.RasterizerPoints[i + 0].Normals / depths.data[0] * weights.x;
					normal += model.RasterizerPoints[i + 1].Normals / depths.data[1] * weights.y;
					normal += model.RasterizerPoints[i + 2].Normals / depths.data[2] * weights.z;
					normal *= depth;

					auto color = model.shader.PixelColour(texCoord, normal);
					uint32_t color_u32 = (static_cast<uint32_t>(255) << 24) | (static_cast<uint32_t>(color.r) << 16) |
						(static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b) << 0);

					//target.Buffers[x + y * target.Width] = (static_cast<uint32_t>(255) << 24) | (static_cast<uint32_t>(color.r) << 16) |
					//	(static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.b) << 0);

					//target.DepthBuffer[x + y * target.Width] = depth;


					AtomicDepthTestAndWrite(target.DepthBuffer, target.Buffers, x + y * target.Width, depth, color_u32);
				}
			}
		}
	}
}

static void Update(float time) {

}

static float CalculateDollyZoomFov(float fovInitial, float zPosInitial, float zPosCurrent) {
	float desiredHalfHeight = std::tanf(fovInitial / 2) * zPosInitial  / zPosCurrent;
	return atanf(desiredHalfHeight) * 2;
}



int main(int argc, char** argv) {
	cout << "Hello, software rendering!" << endl;

	struct mfb_window* window = mfb_open_ex("Software Renderer", width, height, MFB_WF_RESIZABLE);
	if (window == NULL)
		return 0;

	buffers = std::vector<uint32_t>(width * height, 0);
	image = std::vector<float3>(width * height, float3(0.0f, 0.0f, 0.0f));

	float3 dirToSun = float3::Normalize(float3(0.3, 1.0, 0.6));
	auto BoxTextureShader = TextureShader::CreateShader("box.png");
	auto PlaneTextureShader = TextureShader::CreateShader("plane.png");

	auto DragonLitShader = LitShader::CreateShader("dragon.png", dirToSun);

	auto BoxModel = Model<TextureShader>::LoadFromObj("box", BoxTextureShader);
	auto BoxModel2 = Model<TextureShader>::LoadFromObj("box", BoxTextureShader);
	auto BoxModel3 = Model<TextureShader>::LoadFromObj("box", BoxTextureShader);
	auto BoxModel4 = Model<TextureShader>::LoadFromObj("box", BoxTextureShader);

	auto PlaneModel = Model<TextureShader>::LoadFromObj("plane", PlaneTextureShader);

	auto MonkeyModel = Model<LitShader>::LoadFromObj("dragon", DragonLitShader);

	auto Target = RenderTarget(width, height);
	auto boxTransform = Transform(float3(-5.0f, 0, 10.0f));
	auto boxTransform2 = Transform(float3(5.0f, 0.0f, 10.0f));
	auto boxTransform3 = Transform(float3(5.0f, 0, 0.0f));
	auto boxTransform4 = Transform(float3(-5.0f, 0.0f, 0.0f));
	auto planeTransform = Transform(float3(0, 1.0f, 0), float3(2.0, 2.0, 1.0));
	auto monkeyTransform = Transform(float3(0, 0, 5.0f), float3(1.5, 1.5, 1.5));
	auto cam = Camera(float3(0, 0, 0));

	auto lastTime = std::chrono::steady_clock::now();
	auto currentTime = std::chrono::steady_clock::now();

	auto endTime = std::chrono::steady_clock::now();
	auto deltaTime_ms = std::chrono::duration<float, std::milli>(endTime - currentTime);
	float deltaTime = 0.0f;

	const uint8_t* keys = mfb_get_key_buffer(window);

	int currentMouseX = mfb_get_mouse_x(window);
	int currentMouseY = mfb_get_mouse_y(window);

	auto windowHandle = getActiveWindow();
	mfb_show_cursor(window, false);

	mfb_update_state state;
	do {
		endTime = std::chrono::steady_clock::now();
		deltaTime_ms = endTime - currentTime;
		deltaTime = deltaTime_ms.count() / 1000.0f;

		keys = mfb_get_key_buffer(window);

		const float mouseSensitivity = 0.5f;
		auto& camTransform = cam.transform;

		currentMouseX = mfb_get_mouse_x(window);
		currentMouseY = mfb_get_mouse_y(window);

		centerMouse(windowHandle, width, height);

		//cout << currentMouseX << ' ' << currentMouseY << '\n';
		
		float2 mouseDelta = float2(static_cast<float>(currentMouseX - width / 2), static_cast<float>(currentMouseY - height / 2)) / Target.Width * mouseSensitivity;
		//float2 mouseDelta = float2(0.0f, 0.0f);

		camTransform.Pitch = clamp(camTransform.Pitch - mouseDelta.y, degToRad(-85), degToRad(85));
		camTransform.Yaw -= mouseDelta.x;

		const float camSpeed = 2.5f;
		float3 moveDelta = float3();
		auto camDirection = camTransform.GetBasisVectors();

		if (keys[MFB_KB_KEY_W]) {
			moveDelta += camDirection.khat;
		}
		if (keys[MFB_KB_KEY_S]) {
			moveDelta -= camDirection.khat;
		}
		if (keys[MFB_KB_KEY_A]) {
			moveDelta -= camDirection.ihat;
		}
		if (keys[MFB_KB_KEY_D]) {
			moveDelta += camDirection.ihat;
		}

		camTransform.Position += float3::Normalize(moveDelta) * camSpeed * deltaTime;
		//camTransform.Position += float3() * camSpeed * deltaTime;
		camTransform.Position.y = -1.0f;

		//Update(deltaTime);

		currentTime = std::chrono::steady_clock::now();

		Target.Clear();
		Render(BoxModel, boxTransform, Target, cam);
		Render(BoxModel2, boxTransform2, Target, cam);
		Render(BoxModel3, boxTransform3, Target, cam);
		Render(BoxModel4, boxTransform4, Target, cam);
		Render(PlaneModel, planeTransform, Target, cam);
		Render(MonkeyModel, monkeyTransform, Target, cam);

		lastTime = std::chrono::steady_clock::now();

		std::chrono::duration<float, std::milli> duration = lastTime - currentTime;
		cout << "Frame: " << frame++ << " Time: " << duration.count() << "ms" << " delta_time: " << deltaTime << endl;

		state = mfb_update_ex(window, Target.Buffers.data(), width, height);

		if (state != MFB_STATE_OK)
			break;
	} while (mfb_wait_sync(window));

	//CreateTestImage();
	return 0;
}