#include <iostream>
using namespace std;

#include "Maths.h"
#include <vector>
#include <fstream>
#include <string>
#include <cstdint>

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

static void CreateTestImage() {
	constexpr auto width = 64;
	constexpr auto height = 64;

	std::vector<std::vector<float3>> image(width, std::vector<float3> (height, 0));

	float2 a = float2(0.2f * width, 0.2f * height);
	float2 b = float2(0.7f * width, 0.4f * height);
	float2 c = float2(0.4f * width, 0.8f * height);

    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {

			float2 p = float2(static_cast<float>(x), static_cast<float>(y));

			bool inside = PointInTriangle(a, b, c, p);

			if (inside) image[x][y] = float3(0, 0, 1);
        }
    }

    WriteImageToFile(image, "test", width, height);
}

int main(int argc, char** argv) {
	cout << "Hello, software rendering!" << endl;

	CreateTestImage();
}