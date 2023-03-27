#pragma once

#include <string>

#include "components/simple_scene.h"
#include "core/gpu/frame_buffer.h"


namespace m2
{
	class Tema2 : public gfxc::SimpleScene
	{
	public:
		Tema2();
		~Tema2();

		void Init() override;

	private:
		void FrameStart() override;
		void Update(float deltaTimeSeconds) override;
		void FrameEnd() override;

		void OnInputUpdate(float deltaTime, int mods) override;
		void OnKeyPress(int key, int mods) override;
		void OnKeyRelease(int key, int mods) override;
		void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
		void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
		void OnWindowResize(int width, int height) override;

		void OpenDialog();
		void OnFileSelected(const std::string& fileName);

		// Processing effects
		void Median(int radius);
		void Cartoon(unsigned char low, unsigned char high, unsigned int colors);
		void SaveImage(const std::string& fileName);
		void MedianCut(unsigned char* image, unsigned int channels, unsigned int width, unsigned int height, std::vector<std::tuple<unsigned char, unsigned char, unsigned char, unsigned int>>& pixels, unsigned int depth);

	private:
		Texture2D* originalImage;
		Texture2D* processedImage;

		int outputMode;
		bool gpuProcessing;
		bool saveScreenToImage;
	};
}   // namespace m2
