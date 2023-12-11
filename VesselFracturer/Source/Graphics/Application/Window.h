#pragma once

#include "Utilities/Singleton.h"

/**
*	@file Window.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/09/2019
*/

/**
*	@brief Main window of the application, provided by GLFW.
*/
class Window: public Singleton<Window>
{
	friend class Singleton<Window>;

protected:
	enum Codes { NOT_LOADED, SUCCESSFUL_LOAD, UNSUCCESSFUL_LOAD};

	GLFWwindow* _window;								//!< Loaded GLFW window, if any
	uint8_t		_windowState;							//!< Code of window state as a reference for event cycle petitions

protected:
	/**
	*	@brief Private constructor as this class implement the Singleton pattern.
	*/
	Window();

	/**
	*	@brief Prepares OpenGL default state.
	*/
	void setupOpenGL();

public:
	/**
	*	@brief Initializes the window resources and the GLFW context. Note: it is not needed to ensure this method is called just once
	*		   as the application enters a loop in here.
	*	@param openGL4Version Version of OpenGL 4 which is required for this application [0, 6].
	*	@return Success of initialization.
	*/
	bool load(const uint8_t openGL4Version = 6);

	/**
	*	@brief Terminates the GLFW context and destroys the window.
	*/
	void terminate();
};

