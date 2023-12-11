#include "stdafx.h"

#include "Graphics/Application/Window.h"
#include "Graphics/Application/Fragmentation.h"
#include "Graphics/Core/ComputeShader.h"
#include "Graphics/Core/FragmentationProcedure.h"
#include <windows.h>						// DWORD is undefined otherwise

#define DATASET_GENERATION false

// Laptop support. Use NVIDIA graphic card instead of Intel
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int argc, char* argv[])
{
	srand(time(nullptr));

	std::cout << "__ Starting fragmentation __" << std::endl;

	Window* window = Window::getInstance();
	window->load();

	Fragmentation* fragmentation = new Fragmentation;

#if !DATASET_GENERATION
			//Renderer::getInstance()->getCurrentScene()->load();
			//window->startRenderingCycle();
#else
			//FragmentationProcedure procedure;
			//Fragmentation* scene = dynamic_cast<Fragmentation*>(Renderer::getInstance()->getCurrentScene());
			//scene->generateDataset(procedure, procedure._folder, procedure._searchExtension, procedure._destinationFolder);
#endif

	system("pause");

	return 0;
}
