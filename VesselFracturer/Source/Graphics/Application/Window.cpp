#include "stdafx.h"
#include "Window.h"

#include "Graphics/Core/ComputeShader.h"

/// [Protected methods]

Window::Window(): Singleton(), _window(nullptr), _windowState(NOT_LOADED)
{
}

void Window::setupOpenGL()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	ComputeShader::initializeMaximumSize();			// Once the context is ready we can query for maximum work group size
}

/// [Public methods]

bool Window::load(const uint8_t openGL4Version)
{
	if (glfwInit() != GLFW_TRUE)
		return false;

	glfwWindowHint(GLFW_SAMPLES, 4);										// Antialiasing
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);			// OpenGL Core Profile 4.5
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glm::clamp((int) openGL4Version, 1, 6));

	_window = glfwCreateWindow(1, 1, "Fragmentation", nullptr, nullptr);

	if (_window == nullptr) 
	{												// Window initialization could fail in case the computer doesn't meet the required specs
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(_window);										// From now on the window uses the context with the properties cited previously
	glfwSwapInterval(1);

	glewExperimental = true;			// !! GLFW -> GLEW; order matters
	if (glewInit() != GLEW_OK) 
	{
		glfwTerminate();	

		return false;
	}

	std::cout << glGetString(GL_RENDERER) << std::endl;						// 3D context properties
	std::cout << glGetString(GL_VENDOR) << std::endl;
	std::cout << glGetString(GL_VERSION) << std::endl;
	std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

	this->setupOpenGL();

	_windowState = SUCCESSFUL_LOAD;	

	return true;
}

void Window::terminate()
{
	if (_windowState != SUCCESSFUL_LOAD) return;

	glfwDestroyWindow(_window);							// Free GLFW resources
	glfwTerminate();
}
