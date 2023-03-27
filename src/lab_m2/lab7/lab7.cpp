#include "lab_m2/lab7/lab7.h"

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


Lab7::Lab7()
{
	outputMode = 0;
	gpuProcessing = false;
	saveScreenToImage = false;
	window->SetSize(600, 600);
}


Lab7::~Lab7()
{
}


void Lab7::Init()
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

	std::string shaderPath = PATH_JOIN(window->props.selfDir, SOURCE_PATH::M2, "lab7", "shaders");

	// Create a shader program for particle system
	{
		Shader* shader = new Shader("ImageProcessing");
		shader->AddShader(PATH_JOIN(shaderPath, "VertexShader.glsl"), GL_VERTEX_SHADER);
		shader->AddShader(PATH_JOIN(shaderPath, "FragmentShader.glsl"), GL_FRAGMENT_SHADER);

		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}


void Lab7::FrameStart()
{
}


void Lab7::Update(float deltaTimeSeconds)
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


void Lab7::FrameEnd()
{
	DrawCoordinateSystem();
}


void Lab7::OnFileSelected(const std::string& fileName)
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


void Lab7::GrayScale()
{
	unsigned int channels = originalImage->GetNrChannels();
	unsigned char* data = originalImage->GetImageData();
	unsigned char* newData = processedImage->GetImageData();

	if (channels < 3)
		return;

	glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());

	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);

			// Reset save image data
			char value = static_cast<char>(data[offset + 0] * 0.2f + data[offset + 1] * 0.71f + data[offset + 2] * 0.07);
			memset(&newData[offset], value, 3);
		}
	}

	processedImage->UploadNewData(newData);
}

void Lab7::Mean(int radius)
{
	unsigned int channels = originalImage->GetNrChannels();
	unsigned char* data = originalImage->GetImageData();
	unsigned char* newData = processedImage->GetImageData();

	if (channels < 3)
		return;

	glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());
	memcpy(newData, data, channels * imageSize.x * imageSize.y);

	for (int i = radius; i < imageSize.y - radius; i++)
	{
		for (int j = radius; j < imageSize.x - radius; j++)
		{
			int offset = channels * (i * imageSize.x + j);
			int r = 0, g = 0, b = 0;

			for (int y = i - radius; y <= i + radius; y++)
				for (int x = j - radius; x <= j + radius; x++) {
					int offsetN = channels * (y * imageSize.x + x);
					r += data[offsetN + 0];
					g += data[offsetN + 1];
					b += data[offsetN + 2];
				}
			newData[offset + 0] = static_cast<char>(r / (2 * radius + 1) / (2 * radius + 1));
			newData[offset + 1] = static_cast<char>(g / (2 * radius + 1) / (2 * radius + 1));
			newData[offset + 2] = static_cast<char>(b / (2 * radius + 1) / (2 * radius + 1));
		}
	}
	processedImage->UploadNewData(newData);

}

void Lab7::Gaussian()
{
	unsigned int channels = originalImage->GetNrChannels();
	unsigned char* data = originalImage->GetImageData();
	unsigned char* newData = processedImage->GetImageData();

	if (channels < 3)
		return;

	float kernel[25] = { 1, 4, 7, 4, 1, 4, 16, 26, 16, 4, 7, 26, 41, 26, 7, 4, 16, 26, 16, 4, 1, 4, 7, 4, 1 };

	glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());
	memcpy(newData, data, channels * imageSize.x * imageSize.y);

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
					r += kernel[l] * data[offsetN + 0];
					g += kernel[l] * data[offsetN + 1];
					b += kernel[l] * data[offsetN + 2];
					l++;
				}
			newData[offset + 0] = static_cast<char>(r / 273);
			newData[offset + 1] = static_cast<char>(g / 273);
			newData[offset + 2] = static_cast<char>(b / 273);
		}
	}
	processedImage->UploadNewData(newData);
}

void Lab7::Median(int radius)
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

void Lab7::Canny()
{
	unsigned int channels = originalImage->GetNrChannels();
	unsigned char* data = originalImage->GetImageData();
	unsigned char* newData = processedImage->GetImageData();

	if (channels < 3)
		return;

	glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());
	memcpy(newData, data, channels * imageSize.x * imageSize.y);

	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);

			// Reset save image data
			char value = static_cast<char>(data[offset + 0] * 0.2f + data[offset + 1] * 0.71f + data[offset + 2] * 0.07);
			memset(&newData[offset], value, 3);
		}
	}

	float kernel[25] = { 1, 4, 7, 4, 1, 4, 16, 26, 16, 4, 7, 26, 41, 26, 7, 4, 16, 26, 16, 4, 1, 4, 7, 4, 1 };
	unsigned char* newnewData = (unsigned char*) malloc(channels * imageSize.x * imageSize.y);

	memcpy(newnewData, newData, channels * imageSize.x * imageSize.y);

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
					r += kernel[l] * newData[offsetN + 0];
					g += kernel[l] * newData[offsetN + 1];
					b += kernel[l] * newData[offsetN + 2];
					l++;
				}
			newnewData[offset + 0] = static_cast<char>(r / 273);
			newnewData[offset + 1] = static_cast<char>(g / 273);
			newnewData[offset + 2] = static_cast<char>(b / 273);
		}
	}

	unsigned char* newnewnewData = (unsigned char*)malloc(channels * imageSize.x * imageSize.y);
	memcpy(newnewnewData, newnewData, channels * imageSize.x * imageSize.y);

	float gx[] = { 1, 0, -1, 2, 0, -2, 1, 0, -1 };
	float gy[] = { 1, 2, 1, 0, 0, 0, -1, -2, -1 };

	for (int i = 1; i < imageSize.y - 1; i++)
		for (int j = 1; j < imageSize.x - 1; j++) {

			int offset = channels * (i * imageSize.x + j);

			float dx = 0, dy = 0;
			int l = 0;
			for (int y = i - 1; y <= i + 1; y++) {
				for (int x = j - 1; x <= j + 1; x++) {
					int offsetN = channels * (y * imageSize.x + x);
					dx += newnewData[offsetN + 0] * gx[l];
					dy += newnewData[offsetN + 0] * gy[l];
					l++;
				}
			}
			memset(&newData[offset], static_cast<char>(sqrt(dx * dx + dy * dy)), 3);
		}
	for (int i = 0; i < imageSize.y; i++) {
		for (int j = 0;j < imageSize.x; j++) {
			int offset = channels * (i * imageSize.x + j);

			char newValue = newnewnewData[offset + 0];
			if (newValue < 40) {
				newValue = 40;
			}
			else if (newValue > 200) {
				newValue = 200;
			}
			memset(&newnewnewData[offset], static_cast<char>(newValue), 3);
		}
	}
	processedImage->UploadNewData(newnewnewData);
}

void Lab7::SaveImage(const std::string& fileName)
{
	cout << "Saving image! ";
	processedImage->SaveToFile((fileName + ".png").c_str());
	cout << "[Done]" << endl;
}


void Lab7::OpenDialog()
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


void Lab7::OnInputUpdate(float deltaTime, int mods)
{
	// Treat continuous update based on input
}


void Lab7::OnKeyPress(int key, int mods)
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
					GrayScale();
					break;
				case 2:
					Mean(3);
					break;
				case 3:
					Mean(3);
					break;
				case 4:
					Gaussian();
					break;
				case 5:
					Median(3);
					break;
				case 6:
					Canny();
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


void Lab7::OnKeyRelease(int key, int mods)
{
	// Add key release event
}


void Lab7::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY)
{
	// Add mouse move event
}


void Lab7::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods)
{
	// Add mouse button press event
}


void Lab7::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods)
{
	// Add mouse button release event
}


void Lab7::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY)
{
	// Treat mouse scroll event
}


void Lab7::OnWindowResize(int width, int height)
{
	// Treat window resize event
}
