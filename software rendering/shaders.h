#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "stb_image.h"

#include "Maths.h"

class Texture {
public:
	uint32_t Width;
	uint32_t Height;
	float fWidthMinus1;
	float fHeightMinus1;
	std::vector<float3> image;

	Texture(uint32_t width, uint32_t height, const std::vector<float3>& image) : Width(width), Height(height),
		image(image), fWidthMinus1(static_cast<float>(width - 1)), fHeightMinus1(static_cast<float>(height - 1)) {
	}

	float3 Sample(float2& texCoord) {
		texCoord = float2::Saturate(texCoord);

		//auto x = std::lroundf(texCoord.x * (Width - 1));
		//auto y = std::lroundf(texCoord.y * (Height - 1));

		auto x = static_cast<int>(texCoord.x * fWidthMinus1);
		auto y = static_cast<int>(texCoord.y * fHeightMinus1);

		//std::cout << x << ' ' << y << '\n';
		//std::cout << image.size() << '\n';
		//std::cout << Width << ' ' << Height << '\n';

		//return this->image[y * Width + x];
		return image[y * Width + x];
	}
};




typedef struct _Image {
	int width;
	int height;
	std::vector<float3> data;
} Image;

static Image LoadPngFile(const std::string& filename) {
	//cout << "Loading texture: " << filename << '\n';
	std::ifstream file(filename, std::ios::binary | std::ios::ate);

	if (file.is_open()) {
		auto size = file.tellg();

		file.seekg(0, std::ios::beg);

		std::vector<unsigned char> buffer(size);
		file.read((char*)buffer.data(), size);

		int width = 0;
		int height = 0;
		int channel = 0;

		auto mem = stbi_load_from_memory(buffer.data(), size, &width, &height, &channel, 0);
		std::vector<float3> image(width * height);

		//std::cout << "Loaded texture: " << filename << ' ' << width << ' ' << height << ' ' << channel << '\n';

		for (size_t i = 0; i < width * height * channel; i++)
		{
			image[i / channel].data[i % channel] = mem[i];
		}

		return Image{ .width = width, .height = height, .data = image };
	}

	std::abort();
}


template <typename Derived>
class Shader {
public:
	float3 PixelColour(float2& texCoord, float3& normal) {
		return static_cast<Derived*>(this)->PixelColour(texCoord, normal);
	}
};

class TextureShader : public Shader<TextureShader> {
public:
	Texture texture;

	TextureShader(Texture texture) : texture(texture) {}

	static TextureShader CreateShader(const std::string& filename) {
		auto image = LoadPngFile(filename);

		auto texture = Texture(image.width, image.height, image.data);

		return TextureShader(texture);
	}

	float3 PixelColour(float2& texCoord, float3& normal) {
		return texture.Sample(texCoord);
	}
};

class LitShader : public Shader<LitShader> {
public:
	Texture texture;
	float3 DirectionToLight;

	LitShader(Texture texture, float3 directionToLight) : texture(texture), DirectionToLight(directionToLight) {}

	static LitShader CreateShader(const std::string& filename, float3 directionToLight) {
		auto image = LoadPngFile(filename);
		return LitShader(Texture(image.width, image.height, image.data), directionToLight);
	}

	float3 PixelColour(float2& texCoord, float3& normal) {
		normal = float3::Normalize(normal);
		float lightIntensity = (float3::Dot(normal, DirectionToLight) + 1) * 0.5f;
		return float3(1.0, 1.0, 1.0) * lightIntensity * 255.0;
	}
};