#include "lab_m2/tema2/tema2.h"

#include <vector>
#include <iostream>
#include <cmath>

#include "pfd/portable-file-dialogs.h"

using namespace std;
using namespace m2;


/*
 *  To find out more about `FrameStart`, `Update`, `FrameEnd`
 *  and the order in which they are called, see `world.cpp`.
 */


Tema2::Tema2()
{
	outputMode = 0;
	gpuProcessing = false;
	saveScreenToImage = false;
	window->SetSize(600, 600);
}


Tema2::~Tema2()
{
}


void Tema2::Init()
{
	// Load default texture fore imagine processing
	originalImage = TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "cube", "pos_x.png"), nullptr, "image", true, true);
	processedImage = TextureManager::LoadTexture(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "cube", "pos_x.png"), nullptr, "newImage", true, true);

	{
		Mesh* mesh = new Mesh("quad");
		mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "quad.obj");
		mesh->UseMaterials(false);
		meshes[mesh->GetMeshID()] = mesh;
	}

	std::string shaderPath = PATH_JOIN(window->props.selfDir, SOURCE_PATH::M2, "tema2", "shaders");

	// Create a shader program for particle system
	{
		Shader* shader = new Shader("ImageProcessing");
		shader->AddShader(PATH_JOIN(shaderPath, "VertexShader.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "FragmentShader.glsl"), GL_FRAGMENT_SHADER);

		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}


void Tema2::FrameStart()
{
}


void Tema2::Update(float deltaTimeSeconds)
{
	ClearScreen();

	auto shader = shaders["ImageProcessing"];
	shader->Use();

	if (saveScreenToImage)
	{
		window->SetSize(originalImage->GetWidth(), originalImage->GetHeight());
	}

	int flip_loc = shader->GetUniformLocation("flipVertical");
	glUniform1i(flip_loc, saveScreenToImage ? 0 : 1);

	int screenSize_loc = shader->GetUniformLocation("screenSize");
	glm::ivec2 resolution = window->GetResolution();
	glUniform2i(screenSize_loc, resolution.x, resolution.y);

	int outputMode_loc = shader->GetUniformLocation("outputMode");
	glUniform1i(outputMode_loc, outputMode);

	int locTexture = shader->GetUniformLocation("textureImage");
	glUniform1i(locTexture, 0);

	auto textureImage = (gpuProcessing == true) ? originalImage : processedImage;
	textureImage->BindToTextureUnit(GL_TEXTURE0);

	RenderMesh(meshes["quad"], shader, glm::mat4(1));

	if (saveScreenToImage)
	{
		saveScreenToImage = false;

		GLenum format = GL_RGB;
		if (originalImage->GetNrChannels() == 4)
		{
			format = GL_RGBA;
		}

		glReadPixels(0, 0, originalImage->GetWidth(), originalImage->GetHeight(), format, GL_UNSIGNED_BYTE, processedImage->GetImageData());
		processedImage->UploadNewData(processedImage->GetImageData());
		SaveImage("shader_processing_" + std::to_string(outputMode));

		float aspectRatio = static_cast<float>(originalImage->GetWidth()) / originalImage->GetHeight();
		window->SetSize(static_cast<int>(600 * aspectRatio), 600);
	}
}


void Tema2::FrameEnd()
{
	DrawCoordinateSystem();
}


void Tema2::OnFileSelected(const std::string& fileName)
{
	if (fileName.size())
	{
		std::cout << fileName << endl;
		originalImage = TextureManager::LoadTexture(fileName, nullptr, "image", true, true);
		processedImage = TextureManager::LoadTexture(fileName, nullptr, "newImage", true, true);

		float aspectRatio = static_cast<float>(originalImage->GetWidth()) / originalImage->GetHeight();
		window->SetSize(static_cast<int>(600 * aspectRatio), 600);
	}
}

void Tema2::Median(int radius)
{
	unsigned int channels = originalImage->GetNrChannels();
	unsigned char* data = originalImage->GetImageData();
	unsigned char* newData = processedImage->GetImageData();

	if (channels < 3)
		return;

	glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());
	memcpy(newData, data, channels * imageSize.x * imageSize.y);
	std::vector<std::tuple<char, char, char>> sorted;

	for (int i = radius; i < imageSize.y - radius; i++)
	{
		for (int j = radius; j < imageSize.x - radius; j++)
		{
			int offset = channels * (i * imageSize.x + j);
			int r = 0, g = 0, b = 0;

			for (int y = i - radius; y <= i + radius; y++)
				for (int x = j - radius; x <= j + radius; x++) {
					int offsetN = channels * (y * imageSize.x + x);
					r = data[offsetN + 0];
					g = data[offsetN + 1];
					b = data[offsetN + 2];
					sorted.push_back(make_tuple(static_cast<char>(r), static_cast<char>(g), static_cast<char>(b)));
				}
			std::nth_element(sorted.begin(), sorted.begin() + (2 * radius + 1) * (2 * radius + 1) / 2, sorted.end(),
				[](const tuple<char, char, char>& a, const tuple<char, char, char>& b) -> bool {
					char g1 = static_cast<char>(get<0>(a) * 0.2f + get<1>(a) * 0.71f + get<2>(a) * 0.07);
			char g2 = static_cast<char>(get<0>(b) * 0.2f + get<1>(b) * 0.71f + get<2>(b) * 0.07);

			return g1 < g2;
				});
			newData[offset + 0] = get<0>(sorted[(2 * radius + 1) * (2 * radius + 1) / 2]);
			newData[offset + 1] = get<1>(sorted[(2 * radius + 1) * (2 * radius + 1) / 2]);
			newData[offset + 2] = get<2>(sorted[(2 * radius + 1) * (2 * radius + 1) / 2]);
			sorted.clear();
		}
	}
	processedImage->UploadNewData(newData);
}

void Tema2::MedianCut(unsigned char* image, unsigned int channels, unsigned int width, unsigned int height, vector<tuple<unsigned char, unsigned char, unsigned char, unsigned int>>& pixels, unsigned int depth)
{
	if (!pixels.size())
		return;
	if (!depth)
	{
		unsigned int sum_r = 0, sum_g = 0, sum_b = 0;
		for (auto& pixel : pixels) {
			sum_r += get<0>(pixel);
			sum_g += get<1>(pixel);
			sum_b += get<2>(pixel);
		}
		sum_r = sum_r / pixels.size();
		sum_g = sum_g / pixels.size();
		sum_b = sum_b / pixels.size();
		for (auto& pixel : pixels) {
			image[get<3>(pixel) + 0] = sum_r;
			image[get<3>(pixel) + 1] = sum_g;
			image[get<3>(pixel) + 2] = sum_b;
		}
		return;
	}
	int max_r = 0, max_g = 0, max_b = 0, min_r = 255, min_g = 255, min_b = 255;
	for (const auto& pixel : pixels) {
		if (get<0>(pixel) > max_r) {
			max_r = get<0>(pixel);
		}
		if (get<1>(pixel) > max_g) {
			max_g = get<1>(pixel);
		}
		if (get<2>(pixel) > max_b) {
			max_b = get<2>(pixel);
		}

		if (get<0>(pixel) < min_r) {
			min_r = get<0>(pixel);
		}
		if (get<1>(pixel) < min_g) {
			min_g = get<1>(pixel);
		}
		if (get<2>(pixel) < min_b) {
			min_b = get<2>(pixel);
		}

	}
	int diffs[3] = { max_r - min_r, max_g - min_g, max_b - min_b };
	int sort_channel = distance(diffs, max_element(diffs, diffs + 3));
	sort(pixels.begin(), pixels.end(), [sort_channel](const auto& a, const auto& b) -> bool {
		switch (sort_channel) {
		case 0:
			return get<0>(a) < get<0>(b);
			break;
		case 1:
			return get<1>(a) < get<1>(b);
			break;
		case 2:
			return get<2>(a) < get<2>(b);
			break;
		}
		});
	std::size_t half_size = pixels.size() / 2;
std:vector<tuple<unsigned char, unsigned char, unsigned char, unsigned int>> split_lo(pixels.begin(), pixels.begin() + half_size);
	std::vector<tuple<unsigned char, unsigned char, unsigned char, unsigned int>> split_hi(pixels.begin() + half_size, pixels.end());
	MedianCut(image, channels, width, height, split_lo, depth - 1);
	MedianCut(image, channels, width, height, split_hi, depth - 1);  
}

void Tema2::Cartoon(unsigned char low, unsigned char high, unsigned int colors)
{
	unsigned int channels = originalImage->GetNrChannels();
	unsigned char* data = originalImage->GetImageData();

	if (channels < 3)
		return;

	glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());

	unsigned char* grayscale = (unsigned char*)malloc(channels * imageSize.x * imageSize.y);
	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);

			unsigned char value = static_cast<unsigned char>(data[offset + 0] * 0.2f + data[offset + 1] * 0.71f + data[offset + 2] * 0.07);
			memset(&grayscale[offset], value, 3);
		}
	}

	float kernel[25] = { 1, 4, 7, 4, 1, 4, 16, 26, 16, 4, 7, 26, 41, 26, 7, 4, 16, 26, 16, 4, 1, 4, 7, 4, 1 };
	unsigned char* gaussian = (unsigned char*)malloc(channels * imageSize.x * imageSize.y);

	memcpy(gaussian, grayscale, channels * imageSize.x * imageSize.y);

	for (int i = 2; i < imageSize.y - 2; i++)
	{
		for (int j = 2; j < imageSize.x - 2; j++)
		{
			int offset = channels * (i * imageSize.x + j);
			int r = 0, g = 0, b = 0;
			int l = 0;

			for (int y = i - 2; y <= i + 2; y++)
				for (int x = j - 2; x <= j + 2; x++) {
					int offsetN = channels * (y * imageSize.x + x);
					r += kernel[l] * grayscale[offsetN + 0];
					g += kernel[l] * grayscale[offsetN + 1];
					b += kernel[l] * grayscale[offsetN + 2];
					l++;
				}

			gaussian[offset + 0] = static_cast<unsigned char>(r / 273);
			gaussian[offset + 1] = static_cast<unsigned char>(g / 273);
			gaussian[offset + 2] = static_cast<unsigned char>(b / 273);
		}
	}

	unsigned char* sobel_magnitude = (unsigned char*)malloc(channels * imageSize.x * imageSize.y);
	memcpy(sobel_magnitude, gaussian, channels * imageSize.x * imageSize.y);

	unsigned char* sobel_direction = (unsigned char*)malloc(channels * imageSize.x * imageSize.y);
	memcpy(sobel_direction, gaussian, channels * imageSize.x * imageSize.y);

	float gx[] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
	float gy[] = { 1, 2, 1, 0, 0, 0, -1, -2, -1 };

	for (int i = 1; i < imageSize.y - 1; i++)
		for (int j = 1; j < imageSize.x - 1; j++) {

			int offset = channels * (i * imageSize.x + j);

			float dx = 0, dy = 0;
			int l = 0;
			for (int y = i - 1; y <= i + 1; y++) {
				for (int x = j - 1; x <= j + 1; x++) {
					int offsetN = channels * (y * imageSize.x + x);
					dx += gaussian[offsetN + 0] * gx[l];
					dy += gaussian[offsetN + 0] * gy[l];
					l++;
				}
			}
			float angle;
			if (abs(dy) <= 0.001) {
				angle = 0;
			}
			else if (abs(dx) <= 0.001) {
				if (dy > 0)
					angle = 90;
				else
					angle = -90;
			}
			else
				angle = atan2(dy, dx) * 180 / M_PI;
			if (angle < 0)
				angle += 180;
			if ((angle >= 0 && angle < 22.5) || (angle >= 157.5 && angle <= 180)) {
				angle = 0;
			}
			else if (angle >= 22.5 && angle < 67.5) {
				angle = 45;
			}
			else if (angle >= 67.5 && angle < 112.5) {
				angle = 90;
			}
			else if (angle >= 112.5 && angle < 157.5) {
				angle = 135;
			}
			memset(&sobel_magnitude[offset], static_cast<unsigned char>(sqrt(dx * dx + dy * dy)), 3);
			memset(&sobel_direction[offset], static_cast<unsigned char>(angle), 3);
		}

	unsigned char* adjusted_magnitude = (unsigned char*)malloc(channels * imageSize.x * imageSize.y);
	memcpy(adjusted_magnitude, sobel_magnitude, channels * imageSize.x * imageSize.y);

	for (int i = 1; i < imageSize.y - 1; i++)
		for (int j = 1; j < imageSize.x - 1; j++) {
			int offset = channels * (i * imageSize.x + j);
			unsigned char A = 255, B = 255, C;

			C = sobel_magnitude[offset + 0];

			switch (sobel_direction[offset + 0]) {
			case 0:
				A = sobel_magnitude[channels * (i * imageSize.x + (j - 1))];
				B = sobel_magnitude[channels * (i * imageSize.x + (j + 1))];
				break;
			case 45:
				A = sobel_magnitude[channels * ((i + 1) * imageSize.x + (j - 1))];
				B = sobel_magnitude[channels * ((i - 1) * imageSize.x + (j + 1))];
				break;
			case 90:
				A = sobel_magnitude[channels * ((i - 1) * imageSize.x + j)];
				B = sobel_magnitude[channels * ((i + 1) * imageSize.x + j)];
				break;
			case 135:
				A = sobel_magnitude[channels * ((i - 1) * imageSize.x + (j - 1))];
				B = sobel_magnitude[channels * ((i + 1) * imageSize.x + (j + 1))];
				break;
			}
			if (C >= A && C >= B)
				memset(&adjusted_magnitude[offset], static_cast<unsigned char>(C), 3);
			else
				memset(&adjusted_magnitude[offset], static_cast<unsigned char>(0), 3);
		}


	unsigned char* canny = (unsigned char*)malloc(channels * imageSize.x * imageSize.y);

	for (int i = 0; i < imageSize.y; i++) {
		for (int j = 0; j < imageSize.x; j++) {
			int offset = channels * (i * imageSize.x + j);

			unsigned char newValue = adjusted_magnitude[offset + 0];

			if (newValue > high) {
				newValue = 255;
			}
			else if (newValue < low) {
				newValue = 0;
			}
			memset(&canny[offset], static_cast<unsigned char>(newValue), 3);
		}
	}
	for (int i = 1; i < imageSize.y - 1; i++) {
		for (int j = 1; j < imageSize.x - 1; j++) {
			int offset = channels * (i * imageSize.x + j);

			unsigned char newValue = canny[offset + 0];

			if (newValue >= low && newValue <= high) {
				unsigned char A, B, C, D, E, F, G, H;

				A = canny[channels * ((i - 1) * imageSize.x + (j - 1))];
				B = canny[channels * ((i - 1) * imageSize.x + j)];
				C = canny[channels * ((i - 1) * imageSize.x + (j + 1))];
				D = canny[channels * (i * imageSize.x + (j - 1))];
				E = canny[channels * (i * imageSize.x + (j + 1))];
				F = canny[channels * ((i + 1) * imageSize.x + (j - 1))];
				G = canny[channels * ((i + 1) * imageSize.x + j)];
				H = canny[channels * ((i + 1) * imageSize.x + (j + 1))];
				if (A == 255 || B == 255 || C == 255 || D == 255 || E == 255 || F == 255 || G == 255 || H == 255) {
					newValue = 255;
				}
				else
					newValue = 0;
			}
			memset(&canny[offset], static_cast<unsigned char>(newValue), 3);
		}
	}

	vector<tuple<unsigned char, unsigned char, unsigned char, unsigned int>> pixels;
	for (int i = 0; i < imageSize.y; i++) {
		for (int j = 0; j < imageSize.x; j++) {
			int offset = channels * (i * imageSize.x + j);
			pixels.push_back(make_tuple(data[offset + 0], data[offset + 1], data[offset + 2], offset));
		}
	}
	unsigned char* cartoon = (unsigned char*)malloc(channels * imageSize.x * imageSize.y);
	memcpy(cartoon, data, channels * imageSize.x * imageSize.y);
	MedianCut(cartoon, channels, imageSize.x, imageSize.y, pixels, colors);

	unsigned char* cartoon_mean = (unsigned char*)malloc(channels * imageSize.x * imageSize.y);
	vector<tuple<unsigned char, unsigned char, unsigned char>> sorted_mean;
	for (int i = 2; i < imageSize.y - 2; i++)
	{
		for (int j = 2; j < imageSize.x - 2; j++)
		{
			int offset = channels * (i * imageSize.x + j);
			int r = 0, g = 0, b = 0;

			for (int y = i - 2; y <= i + 2; y++)
				for (int x = j - 2; x <= j + 2; x++) {
					int offsetN = channels * (y * imageSize.x + x);
					r = cartoon[offsetN + 0];
					g = cartoon[offsetN + 1];
					b = cartoon[offsetN + 2];
					sorted_mean.push_back(make_tuple(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b)));
				}
			std::nth_element(sorted_mean.begin(), sorted_mean.begin() + 25 / 2, sorted_mean.end(),
				[](const auto& a, const auto& b) -> bool {
					char g1 = static_cast<char>(get<0>(a) * 0.2f + get<1>(a) * 0.71f + get<2>(a) * 0.07);
			char g2 = static_cast<char>(get<0>(b) * 0.2f + get<1>(b) * 0.71f + get<2>(b) * 0.07);

			return g1 < g2;
				});
			cartoon_mean[offset + 0] = get<0>(sorted_mean[25 / 2]);
			cartoon_mean[offset + 1] = get<1>(sorted_mean[25 / 2]);
			cartoon_mean[offset + 2] = get<2>(sorted_mean[25 / 2]);
			sorted_mean.clear();
		}
	}

	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);
			if (canny[offset] == 255) {
				memset(&cartoon_mean[offset], 0, 3);
			}
		}
	}

	processedImage->UploadNewData(cartoon_mean);
}

void Tema2::SaveImage(const std::string& fileName)
{
	cout << "Saving image! ";
	processedImage->SaveToFile((fileName + ".png").c_str());
	cout << "[Done]" << endl;
}


void Tema2::OpenDialog()
{
	std::vector<std::string> filters =
	{
		"Image Files", "*.png *.jpg *.jpeg *.bmp",
		"All Files", "*"
	};

	auto selection = pfd::open_file("Select a file", ".", filters).result();
	if (!selection.empty())
	{
		std::cout << "User selected file " << selection[0] << "\n";
		OnFileSelected(selection[0]);
	}
}


/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */


void Tema2::OnInputUpdate(float deltaTime, int mods)
{
	// Treat continuous update based on input
}


void Tema2::OnKeyPress(int key, int mods)
{
	// Add key press event
	if (key == GLFW_KEY_F || key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE)
	{
		OpenDialog();
	}

	if (key == GLFW_KEY_E)
	{
		gpuProcessing = !gpuProcessing;
		cout << "Processing on GPU: " << (gpuProcessing ? "true" : "false") << endl;
	}

	if (key - GLFW_KEY_0 >= 0 && key <= GLFW_KEY_9)
	{
		outputMode = key - GLFW_KEY_0;
		if (!gpuProcessing) {
			switch (outputMode) {
			case 1:
				Cartoon(75, 150, 4);
				break;
			case 0:
				processedImage->UploadNewData(originalImage->GetImageData());
				break;
			}
		}
	}

	if (key == GLFW_KEY_S && mods & GLFW_MOD_CONTROL)
	{
		if (!gpuProcessing)
		{
			SaveImage("processCPU_" + std::to_string(outputMode));
		}
		else {
			saveScreenToImage = true;
		}
	}
}


void Tema2::OnKeyRelease(int key, int mods)
{
	// Add key release event
}


void Tema2::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
	// Add mouse move event
}


void Tema2::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
	// Add mouse button press event
}


void Tema2::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
	// Add mouse button release event
}


void Tema2::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
	// Treat mouse scroll event
}


void Tema2::OnWindowResize(int width, int height)
{
	// Treat window resize event
}
