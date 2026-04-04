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

constexpr auto width = 800;
constexpr auto height = 600;

static void WriteImageToFile(std::vector<std::vector<float3>>& image, const std::string& filename, int width, int height) {
    std::ofstream outFile(filename + ".bmp", std::ios::out | std::ios::binary);
    uint32_t ByteCounts [] = {14, 40, static_cast<uint32_t>(image.size()) * 4};

    if (outFile.is_open()) {
        outFile << "BM"; // "BM"
		uint32_t count = ByteCounts[0] + ByteCounts[1] + ByteCounts[2];
		outFile.write(reinterpret_cast<char *>(&count), sizeof(uint32_t)); // File size
		uint32_t reserved = 0;
		outFile.write(reinterpret_cast<char *>(&reserved), sizeof(uint32_t)); // Reserved
		uint32_t offset = ByteCounts[0] + ByteCounts[1];
		outFile.write(reinterpret_cast<char *>(&offset), sizeof(uint32_t)); // Pixel data offset
		outFile.write(reinterpret_cast<char *>(&ByteCounts[1]), sizeof(uint32_t)); // DIB header size
		outFile.write(reinterpret_cast<char *>(&width), sizeof(uint32_t)); // Image width
		outFile.write(reinterpret_cast<char *>(&height), sizeof(uint32_t)); // Image height
		uint16_t planes = 1;
		outFile.write(reinterpret_cast<char *>(&planes), sizeof(uint16_t)); // Color planes
		uint16_t bitsPerPixel = 8 * 4;
		outFile.write(reinterpret_cast<char *>(&bitsPerPixel), sizeof(uint16_t)); // Bits per pixel
		outFile.write(reinterpret_cast<char *>(&reserved), sizeof(uint32_t)); // Compression method (none)
		outFile.write(reinterpret_cast<char*>(&ByteCounts[2]), sizeof(uint32_t)); // Image size (can be 0 for uncompressed)
		char byte[16]{};
		outFile.write(byte, sizeof(byte));

		for (size_t y = 0; y < height; y++)
		{
			for (size_t x = 0; x < width; x++)
			{
				float3 col = image[x][y];
				uint8_t b = static_cast<uint8_t>(col.b * 255);
				uint8_t g = static_cast<uint8_t>(col.g * 255);
				uint8_t r = static_cast<uint8_t>(col.r * 255);
				uint8_t a = 0;

				outFile.write(reinterpret_cast<char*>(&b), sizeof(uint8_t));
				outFile.write(reinterpret_cast<char*>(&g), sizeof(uint8_t));
				outFile.write(reinterpret_cast<char*>(&r), sizeof(uint8_t));
				outFile.write(reinterpret_cast<char *>(&a), sizeof(uint8_t));
			}
		}

		outFile.flush();
    }
	else {
		cout << "Failed to open file for writing:" << endl;
	}

}

std::vector<float2> points;
std::vector<float2> velocities;
std::vector<float3> triangleCols;
std::vector<float3> image;
std::vector<uint32_t> buffers;


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
		float2 velocity = (float2(dis1(gen), dis2(gen)) - halfSize) * 0.5f;
		velocities[i + 0] = velocity;
		velocities[i + 1] = velocity;
		velocities[i + 2] = velocity;

		triangleCols[i / 3] = float3(static_cast<uint8_t>(disCol(gen)), static_cast<uint8_t>(disCol(gen)), static_cast<uint8_t>(disCol(gen)));
		
	}

    //WriteImageToFile(image, "test", width, height);
}

uint32_t frame = 0;

static void Render() {
	//for (size_t i = 0; i < points.size(); i += 3)
	//{
	//	auto a = points[i + 0];
	//	auto b = points[i + 1];
	//	auto c = points[i + 2];

	//	float minX = std::min(std::min(a.x, b.x), c.x);
	//	float minY = std::min(std::min(a.y, b.y), c.y);
	//	float maxX = std::max(std::max(a.x, b.x), c.x);
	//	float maxY = std::max(std::max(a.y, b.y), c.y);

	//	// Pixel block covering the triangle bounds
	//	
	//	int blockStartx = std::clamp(static_cast<int>(minX), 0, width - 1);
	//	int blockStartY = std::clamp(static_cast<int>(minY), 0, height - 1);
	//	int blockEndx = std::clamp(static_cast<int>(std::ceil(maxX)), 0, width - 1);
	//	int blockEndY = std::clamp(static_cast<int>(std::ceil(maxY)), 0, height - 1);
	//}

	for (size_t y = 0; y < height; y++)
	{
		for (size_t x = 0; x < width; x++)
		{	
			for (size_t i = 0; i < points.size(); i += 3)
			{
				auto a = points[i + 0];
				auto b = points[i + 1];
				auto c = points[i + 2];
				auto p = float2(static_cast<float>(x), static_cast<float>(y));

				if (PointInTriangle(a, b, c, p))
				{
					buffers[x + y * width] = (static_cast<uint32_t>(triangleCols[i / 3].r) << 24) | (static_cast<uint32_t>(triangleCols[i / 3].g) << 16) |
						(static_cast<uint32_t>(triangleCols[i / 3].b) << 8) | 0;
					//image[x + y * height] = triangleCols[i / 3];
					//auto s = a + b;
					//buffers[x + y * height] = 1;
				}
			}
		}
	}

	//for (size_t i = 0; i < image.size(); i++)
	//{
	//	buffers[i] = (static_cast<uint32_t>(image[i].r) << 24) | (static_cast<uint32_t>(image[i].g) <<16) | (static_cast<uint32_t>(image[i].b) << 8);
	//}
}

static void Update(float time) {
	auto time_s = time / 1000.0f;
	for (size_t i = 0; i < points.size(); i++)
	{
		points[i] = points[i] + velocities[i] * time_s;
		if (points[i].x < 0 || points[i].x > width)
			velocities[i].x *= -1.0f;
		if (points[i].y < 0 || points[i].y > height)
			velocities[i].y *= -1.0f;
	}
}


int main(int argc, char** argv) {
	cout << "Hello, software rendering!" << endl;

	struct mfb_window* window = mfb_open_ex("Software Renderer", width, height, MFB_WF_RESIZABLE);
	if (window == NULL)
		return 0;

	buffers = std::vector<uint32_t>(width * height, 0);
	image = std::vector<float3>(width * height, float3(0.0f, 0.0f, 0.0f));

	CreateTestImage();

	auto lastTime = std::chrono::steady_clock::now();
	auto currentTime = std::chrono::steady_clock::now();

	auto endTime = std::chrono::steady_clock::now();
	auto deltaTime = std::chrono::duration<float, std::milli>(endTime - currentTime);

	mfb_update_state state;
	do {
		endTime = std::chrono::steady_clock::now();
		deltaTime = endTime - currentTime;

		Update(deltaTime.count());

		currentTime = std::chrono::steady_clock::now();

		//memset(buffers.data(), 0, buffers.size() * sizeof(uint32_t));
		Render();

		lastTime = std::chrono::steady_clock::now();

		std::chrono::duration<float, milli> duration = lastTime - currentTime;
		cout << "Frame: " << frame++ << " Time: " << duration.count() << "ms" << " delta_time: " << deltaTime.count() << endl;

		state = mfb_update_ex(window, buffers.data(), width, height);

		if (state != MFB_STATE_OK)
			break;
	} while (mfb_wait_sync(window));

	//CreateTestImage();
	return 0;
}