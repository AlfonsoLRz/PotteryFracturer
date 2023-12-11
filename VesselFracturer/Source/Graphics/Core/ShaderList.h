#pragma once

#include "Graphics/Core/ComputeShader.h"
#include "Graphics/Core/GraphicsCoreEnumerations.h"
#include "Utilities/Singleton.h"

/**
*	@file ShaderList.h
*	@authors Alfonso López Ruiz (alr00048@red.ujaen.es)
*	@date 07/20/2019
*/

/**
*	@brief Maintains a list of shaders which are globally accessible.
*/
class ShaderList: public Singleton<ShaderList>
{
	friend class Singleton<ShaderList>;

protected:
	static std::unordered_map<uint8_t, std::string> COMP_SHADER_SOURCE;					//!< Path where we can get each compute shader

protected:
	static std::vector<std::unique_ptr<ComputeShader>>		_computeShader;				//!< Already loaded compute shaders

protected:
	/**
	*	@brief Default constructor.
	*/
	ShaderList();

public:
	/**
	*	@return Compute shader defined by the identifier.
	*/
	ComputeShader* getComputeShader(const ShaderEnum::ComputeShaderType shader);
};

