#pragma once

#include "DataStructures/RegularGrid.h"
#include "Fracturer/FloodFracturer.h"
#include "Fracturer/Seeder.h"
#include "Graphics/Core/AssimpModel.h"

class FractureParameters;
class FragmentationProcedure;
class PointCloud3D;

/**
*	@brief Scene composed of CAD models for the manuscript of this work.
*/
class Fragmentation
{
protected:
	const static std::string TARGET_PATH;						//!< Location of the first mesh in the file system

protected:
	FractureParameters			_fractParameters;				//!< 
	std::vector<Model3D*>		_fractureMeshes;				//!<
	FragmentMetadataBuffer		_fragmentMetadata;				//!< Metadata of the current fragmentation procedure
	bool						_generateDataset;
	AssimpModel*					_mesh;							//!< Jar mesh
	RegularGrid*				_meshGrid;						//!< Mesh regular grid
	PointCloud3D*				_pointCloud;					//!<

protected:
	/**
	*	@brief Allocates memory for the scene.
	*/
	void allocateMemoryDataset(FragmentationProcedure& fractureProcedure);

	/**
	*	@brief Allocates the mesh grid.
	*/
	void allocateMeshGrid(FractureParameters& fractParameters);

	/**
	*	@brief Erase content from a previous fragmentation process.
	*/
	void eraseFragmentContent();

	/**
	*	@brief
	*/
	void exportMetadata(const std::string& filename, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentSize);

	/**
	*	@brief Splits the loaded mesh into fragments through a fracturer algorithm.
	*/
	std::string fractureModel(FractureParameters& fractParameters);

	/**
	*	@brief Saves the whole folder into another one.
	*/
	void launchZipingProcess(const std::string& folder);

	/**
	*	@brief Loads the models which are necessary to render the scene.
	*/
	void loadModels();

	/**
	*	@brief Replaces the currently loaded model.
	*/
	void loadModel(const std::string& path);

	/**
	*	@brief Rebuilds the whole grid to adapt it to a different number of subdivisions.
	*/
	void rebuildGrid(FractureParameters& fractParameters);

public:
	/**
	*	@brief Default constructor.
	*/
	Fragmentation();

	/**
	*	@brief Destructor. Frees memory allocated for 3d models.
	*/
	virtual ~Fragmentation();

	/**
	*	@brief
	*/
	void exportGrid();

	/**
	*	@brief Fractures voxelized model.
	*/
	std::string fractureGrid(std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FractureParameters& fractureParameters);

	/**
	*	@brief Fractures voxelized model.
	*/
	std::string fractureGrid(const std::string& path, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FractureParameters& fractureParameters);

	/**
	*	@brief Generates a dataset of fractured models.
	*/
	void generateDataset(FragmentationProcedure& fractureProcedure, const std::string& folder, const std::string& extension, const std::string& destinationFolder);

	/**
	*	@return Buffer with information of split fragments.
	*/
	std::vector<FragmentationProcedure::FragmentMetadata> getFragmentMetadata() { return _fragmentMetadata; }
};

