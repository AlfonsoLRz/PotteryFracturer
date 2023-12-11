#pragma once

#include "stdafx.h"

/**
*	@brief Set of useful operations for file management.
*/
namespace FileManagement
{
	/**
	*	@brief
	*/
	void clearTokens(std::vector<std::string>& stringTokens, std::vector<float>& floatTokens);

	/**
	*	@brief Searchs for files in a given folder, with a given extension.
	*/
	void searchFiles(const std::string& folder, const std::string& extension, std::vector<std::string>& files);
};

inline void FileManagement::clearTokens(std::vector<std::string>& stringTokens, std::vector<float>& floatTokens)
{
	stringTokens.clear();
	floatTokens.clear();
}

inline void FileManagement::searchFiles(const std::string& folder, const std::string& extension, std::vector<std::string>& files)
{
	for (auto& assetFile : std::filesystem::recursive_directory_iterator(folder))
	{
		if (!assetFile.is_directory() && assetFile.path().generic_string().find(extension) != std::string::npos)
		{
			files.push_back(assetFile.path().generic_string());
		}
	}
}
