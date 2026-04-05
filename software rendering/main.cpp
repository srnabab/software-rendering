#include <iostream>
using namespace std;

#include "Maths.h"
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

constexpr auto width = 800;
constexpr auto height = 600;

std::vector<float2> points;
std::vector<float2> velocities;
std::vector<float3> triangleCols;
std::vector<float3> image;
std::vector<uint32_t> buffers;

uint32_t frame = 0;

class Model {
	public:
		std::vector<float3>Points;
		std::vector<float3>Cols;

		Model(const std::vector<float3>& points, const std::vector<float3>& cols) : Points(points), Cols(cols) {}
};

class RenderTarget {
	public:
		uint32_t Width;
		uint32_t Height;
		float2 Size = float2(Width, Height);

		std::vector<uint32_t> Buffers;
		std::vector<float> DepthBuffer;

		RenderTarget(uint32_t width, uint32_t height) : Width(width), Height(height), Buffers(width* height, 0), DepthBuffer(width * height, 10000000000.0f) {}

		void Clear() {
			memset(Buffers.data(), 0, Buffers.size() * sizeof(uint32_t));
			std::fill(DepthBuffer.begin(), DepthBuffer.end(), 10000000000.0f);
		}
};

static std::vector<float3> LoadObjFile(const std::string& filename) {
	std::vector<float3> allPoints;
	std::vector<float3> trianglePoints;

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
					while (currentSlash <= faceIndexGroups[i].size()) {
						auto nextSlash = faceIndexGroups[i].substr(currentSlash).find('/') + currentSlash;

						//if (nextSlash == currentSlash - 1) break;

						auto pointIndex = stoi(faceIndexGroups[i].substr(currentSlash, nextSlash - currentSlash)) - 1;

						if (i >= 3) {
							trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - (3 * i - 6)));
							//cout << "Triangle vertex: " << i << ' ' << trianglePoints.back().x << ' ' <<
								//trianglePoints.back().y << ' ' << trianglePoints.back().z << '\n';
						}
						if (i >= 3) {
							trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - 2));
							//cout << "Triangle vertex: " << i << ' ' << trianglePoints.back().x << ' ' <<
								//trianglePoints.back().y << ' ' << trianglePoints.back().z << '\n';
						}
						trianglePoints.push_back(allPoints[pointIndex]);

						//cout << "Triangle vertex: " << i << ' ' << trianglePoints.back().x << ' ' <<
							//trianglePoints.back().y << ' ' << trianglePoints.back().z << '\n';

						//currentSlash = nextSlash + 1;
						break;
					}
				}
			}
		}
	}

	//for (size_t i = 0; i < trianglePoints.size(); i++)
	//{
		//cout << "Triangle vertex: " << i << ' ' << trianglePoints[i].x << ' ' << trianglePoints[i].y << ' ' << trianglePoints[i].z << '\n';
	//}
	cout << trianglePoints.size() << '\n';

	return trianglePoints;
}

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

float3 VertexToScreen(float3 vertex, Transform transform, float2 numPixels, float fov) {
	float3 vertex_world = transform.ToWorldPoint(vertex);

	float screenHeight_world = std::tanf(fov / 2) * 2;
	float pixelsPerWorldUnit = numPixels.y / screenHeight_world / vertex_world.z;

	float2 pixelOffset = float2(vertex_world.x, vertex_world.y) * pixelsPerWorldUnit;
	float2 vertex_screen = numPixels / 2.0f + pixelOffset;

	return float3(vertex_screen.x, vertex_screen.y, vertex_world.z);
}


static void Render(Model& model, Transform& transform, RenderTarget& target, float fov) {
	for (size_t i = 0; i < model.Points.size(); i += 3)
	{
		auto a = VertexToScreen(model.Points[i + 0], transform, target.Size, fov);
		auto b = VertexToScreen(model.Points[i + 1], transform, target.Size, fov);
		auto c = VertexToScreen(model.Points[i + 2], transform, target.Size, fov);

		//cout << a.x << ',' << a.y << ' ' << b.x << ',' << b.y << ' ' << c.x << ',' << c.y << '\n';

		float minX = std::min(std::min(a.x, b.x), c.x);
		float minY = std::min(std::min(a.y, b.y), c.y);
		float maxX = std::max(std::max(a.x, b.x), c.x);
		float maxY = std::max(std::max(a.y, b.y), c.y);

		//Pixel block covering the triangle bounds

		int blockStartX = std::clamp(static_cast<int>(minX), 0, static_cast<int>(target.Width - 1));
		int blockStartY = std::clamp(static_cast<int>(minY), 0, static_cast<int>(target.Height - 1));
		int blockEndX = std::clamp(static_cast<int>(std::ceil(maxX)), 0, static_cast<int>(target.Width - 1));
		int blockEndY = std::clamp(static_cast<int>(std::ceil(maxY)), 0, static_cast<int>(target.Height - 1));


		for (size_t y = blockStartY; y < blockEndY; y++)
		{
			for (size_t x = blockStartX; x < blockEndX; x++)
			{
				float2 p = float2(static_cast<float>(x), static_cast<float>(y));
				float3 weights;

				if (PointInTriangle((float2)a, (float2)b, (float2)c, p, weights)) {

					float3 depths = float3(a.z, b.z, c.z);
					float depth = float3::Dot(depths, weights);

					if (depth > target.DepthBuffer[x + y * target.Width]) continue;

					target.Buffers[x + y * target.Width] = (static_cast<uint32_t>(model.Cols[i / 3].r) << 16) | (static_cast<uint32_t>(model.Cols[i / 3].g) << 8) |
						(static_cast<uint32_t>(model.Cols[i / 3].b) << 0);
					target.DepthBuffer[x + y * target.Width] = depth;
				}
			}
		}
	}
}

static void Update(float time) {
	auto time_s = time / 1000.0f;
	for (size_t i = 0; i < points.size(); i++)
	{
		points[i] = points[i] + velocities[i] * time_s;
		if (points[i].x < 0) {
			points[i].x = 0;
			velocities[i].x *= -1.0f;
		}
		else if (points[i].x > width - 1) {
			points[i].x = static_cast<float>(width - 1);
			velocities[i].x *= -1.0f;
		}

		if (points[i].y < 0) {
			points[i].y = 0;
			velocities[i].y *= -1.0f;
		}
		else if (points[i].y > height - 1) {
			points[i].y = static_cast<float>(height - 1);
			velocities[i].y *= -1.0f;
		}
	}
}

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

static float CalculateDollyZoomFov(float fovInitial, float zPosInitial, float zPosCurrent) {
	float desiredHalfHeight = std::tanf(fovInitial / 2) * zPosInitial  / zPosCurrent;
	return atanf(desiredHalfHeight) * 2;
}

static float dagToRad(float degrees) {
	return degrees * (std::numbers::pi / 180.0f);
}

static float radToDag(float radians) {
	return radians * (180.0f / std::numbers::pi);
}


int main(int argc, char** argv) {
	cout << "Hello, software rendering!" << endl;

	struct mfb_window* window = mfb_open_ex("Software Renderer", width, height, MFB_WF_RESIZABLE);
	if (window == NULL)
		return 0;

	buffers = std::vector<uint32_t>(width * height, 0);
	image = std::vector<float3>(width * height, float3(0.0f, 0.0f, 0.0f));

	auto model = LoadObjFile("monkey.obj");

	std::vector<float3> boxCols(model.size() / 3);
	for (size_t i = 0; i < boxCols.size(); i++)
	{
		boxCols[i] = GenRandomColor();
	}

	auto BoxModel = Model(model, boxCols);
	auto Target = RenderTarget(width, height);
	auto transform = Transform(float3(0, 0, 5.0f));
	float fov = 60.0f;

	auto lastTime = std::chrono::steady_clock::now();
	auto currentTime = std::chrono::steady_clock::now();

	auto endTime = std::chrono::steady_clock::now();
	auto deltaTime = std::chrono::duration<float, std::milli>(endTime - currentTime);

	const uint8_t* keys = mfb_get_key_buffer(window);

	mfb_update_state state;
	do {
		endTime = std::chrono::steady_clock::now();
		deltaTime = endTime - currentTime;

		keys = mfb_get_key_buffer(window);

		if (keys[MFB_KB_KEY_0]) {
			transform.Yaw -= 0.5f * deltaTime.count() / 1000.0f;
		}
		if (keys[MFB_KB_KEY_1]) {
			transform.Yaw += 0.5f * deltaTime.count() / 1000.0f;
		}
		if (keys[MFB_KB_KEY_2]) {
			transform.Pitch -= 0.5f * deltaTime.count() / 1000.0f;
		}
		if (keys[MFB_KB_KEY_3]) {
			transform.Pitch += 0.5f * deltaTime.count() / 1000.0f;
		}
		if (keys[MFB_KB_KEY_UP]) {
			transform.Position.z += 3.0f * deltaTime.count() / 1000.0f;

			cout << "Position: " << transform.Position.z << " FOV: " << radToDag(CalculateDollyZoomFov(dagToRad(fov), 5.0f, transform.Position.z)) << '\n';
		}
		if (keys[MFB_KB_KEY_DOWN]) {
			transform.Position.z -= 3.0f * deltaTime.count() / 1000.0f;

			cout << "Position: " << transform.Position.z << " FOV: " << radToDag(CalculateDollyZoomFov(dagToRad(fov), 5.0f, transform.Position.z)) << '\n';
		}


		//Update(deltaTime.count());

		currentTime = std::chrono::steady_clock::now();

		Target.Clear();
		Render(BoxModel, transform, Target, CalculateDollyZoomFov(dagToRad(fov), 5.0f, transform.Position.z));

		lastTime = std::chrono::steady_clock::now();

		std::chrono::duration<float, milli> duration = lastTime - currentTime;
		//cout << "Frame: " << frame++ << " Time: " << duration.count() << "ms" << " delta_time: " << deltaTime.count() << endl;

		state = mfb_update_ex(window, Target.Buffers.data(), width, height);

		if (state != MFB_STATE_OK)
			break;
	} while (mfb_wait_sync(window));

	//CreateTestImage();
	return 0;
}