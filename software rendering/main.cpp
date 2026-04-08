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

#include "Window.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

constexpr auto width = 1200;
constexpr auto height = 900;

std::vector<float2> points;
std::vector<float2> velocities;
std::vector<float3> triangleCols;
std::vector<float3> image;
std::vector<uint32_t> buffers;

uint32_t frame = 0;

class Shader {
public:
	virtual float3 PixelColour(float2 texCoord) = 0;
};

class Texture {
public:
	uint32_t Width;
	uint32_t Height;
	std::vector<float3> image;

	Texture(uint32_t width, uint32_t height, const std::vector<float3>& image) : Width(width), Height(height), image(image) {}

	float3 Sample(float2 texCoord) {
		texCoord = float2::Saturate(texCoord);

		auto x = std::lroundf(std::clamp(texCoord.x, 0.0f, 1.0f) * (Width - 1));
		auto y = std::lroundf(std::clamp(texCoord.y, 0.0f, 1.0f) * (Height - 1));

		//cout << x << ' ' << y << '\n';

		return this->image[y * Width + x];
		//return image[y * Width + x];
	}
};

class TextureShader : public Shader {
public:
	Texture texture;

	TextureShader(Texture texture) : texture(texture) {}

	virtual float3 PixelColour(float2 texCoord) {
		return texture.Sample(texCoord);
	}
};

static TextureShader LoadPngFile(const std::string& filename) {
	cout << "Loading texture: " << filename << '\n';
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

		for (size_t i = 0; i < width * height * channel; i++)
		{
			image[i / channel].data[i % channel] = mem[i];
		}

		return TextureShader(Texture(width, height, image));
	}

	std::abort();
}

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
					while (currentSlash <= faceIndexGroups[i].size()) {
						auto nextSlash = faceIndexGroups[i].substr(currentSlash).find('/') + currentSlash;

						if (times == 3) break;

						auto pointIndex = stoi(faceIndexGroups[i].substr(currentSlash, nextSlash - currentSlash)) - 1;

						if (times == 0) {

							if (i >= 3) {
								trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - (3 * i - 6)));
							}
							if (i >= 3) {
								trianglePoints.push_back(trianglePoints.at(trianglePoints.size() - 2));
							}
							trianglePoints.push_back(allPoints[pointIndex]);
						}
						else if (times == 2) {

							if (i >= 3) {
								texCoord.push_back(texCoord.at(texCoord.size() - (3 * i - 6)));
							}
							if (i >= 3) {
								texCoord.push_back(texCoord.at(texCoord.size() - 2));
							}
							texCoord.push_back(texCoordPoints[pointIndex]);
						}
						else if (times == 1) {

							if (i >= 3) {
								normal.push_back(normal.at(normal.size() - (3 * i - 6)));
							}
							if (i >= 3) {
								normal.push_back(normal.at(normal.size() - 2));
							}
							normal.push_back(normalPoints[pointIndex]);
						}

						times++;
						currentSlash = nextSlash + 1;
						//break;
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

class Model {
public:
	std::vector<float3>Points;
	std::vector<float2>TexCoords;
	std::vector<float3> Normals;
	//std::vector<float3>Cols;
	TextureShader shader;

	//Model(const std::vector<float3>& points, const std::vector<float3>& cols, const TextureShader& shader) : Points(points), Cols(cols), shader(shader) {}
	Model(const std::vector<float3>& points, const TextureShader& shader, const std::vector<float2>& texCoords, const std::vector<float3>& normals) :
		Points(points), shader(shader), TexCoords(texCoords), Normals(normals) {}

	static Model LoadFromObj(const std::string& filename) {
		auto vertices = LoadObjFile(filename + ".obj");

		auto image = LoadPngFile(filename + ".png");

		return Model(vertices.position, image, vertices.texCoord, vertices.normal);
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

	float screenHeight_world = std::tanf(cam.fov / 2) * 2;
	float pixelsPerWorldUnit = numPixels.y / screenHeight_world / vertex_view.z;

	float2 pixelOffset = float2(vertex_view.x, vertex_view.y) * pixelsPerWorldUnit;
	float2 vertex_screen = numPixels / 2.0f + pixelOffset;

	return float3(vertex_screen.x, vertex_screen.y, vertex_view.z);
}


static void Render(Model& model, Transform& transform, RenderTarget& target, Camera cam) {
	for (size_t i = 0; i < model.Points.size(); i += 3)
	{
		auto a = VertexToScreen(model.Points[i + 0], transform, cam, target.Size);
		auto b = VertexToScreen(model.Points[i + 1], transform, cam, target.Size);
		auto c = VertexToScreen(model.Points[i + 2], transform, cam, target.Size);

		if (a.z <= 0 || b.z <= 0 || c.z <= 0) continue;

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
					float depth = 1 / float3::Dot(1 / depths, weights);

					if (depth > target.DepthBuffer[x + y * target.Width]) continue;

					float2 texCoord = float2();
					texCoord += model.TexCoords[i + 0] * weights.x;
					texCoord += model.TexCoords[i + 1] * weights.y;
					texCoord += model.TexCoords[i + 2] * weights.z;

					//target.Buffers[x + y * target.Width] = (static_cast<uint32_t>(model.Cols[i / 3].r) << 16) | (static_cast<uint32_t>(model.Cols[i / 3].g) << 8) |
					//	(static_cast<uint32_t>(model.Cols[i / 3].b) << 0);
					auto color = model.shader.PixelColour(texCoord);
					target.Buffers[x + y * target.Width] = (static_cast<uint32_t>(color.r) << 16) | (static_cast<uint32_t>(color.g) << 8) |
							(static_cast<uint32_t>(color.b) << 0);
					target.DepthBuffer[x + y * target.Width] = depth;
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

	auto BoxModel = Model::LoadFromObj("box");
	auto BoxModel2 = BoxModel;
	auto BoxModel3 = BoxModel;
	auto BoxModel4 = BoxModel;

	auto PlaneModel = Model::LoadFromObj("plane");

	auto MonkeyModel = Model::LoadFromObj("monkey");

	auto Target = RenderTarget(width, height);
	auto boxTransform = Transform(float3(-5.0f, 0, 10.0f));
	auto boxTransform2 = Transform(float3(5.0f, 0.0f, 10.0f));
	auto boxTransform3 = Transform(float3(5.0f, 0, 0.0f));
	auto boxTransform4 = Transform(float3(-5.0f, 0.0f, 0.0f));
	auto planeTransform = Transform(float3(0, 1.0f, 0));
	auto monkeyTransform = Transform(float3(0, 0, 5.0f));
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
		//cout << "Frame: " << frame++ << " Time: " << duration.count() << "ms" << " delta_time: " << deltaTime << endl;

		state = mfb_update_ex(window, Target.Buffers.data(), width, height);

		if (state != MFB_STATE_OK)
			break;
	} while (mfb_wait_sync(window));

	//CreateTestImage();
	return 0;
}