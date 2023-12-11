#include "stdafx.h"
#include "Fragmentation.h"

#include "Geometry/3D/PointCloud3D.h"
#include "Graphics/Core/AssimpModel.h"
#include "Graphics/Core/FractureParameters.h"
#include "Graphics/Core/FragmentationProcedure.h"
#include "progressbar.hpp"
#include "Utilities/ChronoUtilities.h"
#include "Utilities/FileManagement.h"


/// Initialization of static attributes
const std::string Fragmentation::TARGET_PATH = "E:/Research/serapis.obj";

// [Public methods]

Fragmentation::Fragmentation() :
	_generateDataset(false), _mesh(nullptr), _meshGrid(nullptr), _pointCloud(nullptr)
{
	srand(_fractParameters._seed);
	RandomUtilities::initSeed(_fractParameters._seed);
}

Fragmentation::~Fragmentation()
{
	delete _mesh;
	delete _meshGrid;
	delete _pointCloud;
}

void Fragmentation::exportGrid()
{
	_meshGrid->exportGrid();
}

std::string Fragmentation::fractureGrid(std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FractureParameters& fractureParameters)
{
	this->eraseFragmentContent();
	this->rebuildGrid(fractureParameters);
	return this->fractureModel(fractureParameters);
}

std::string Fragmentation::fractureGrid(const std::string& path, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentMetadata, FractureParameters& fractureParameters)
{
	this->eraseFragmentContent();

	if (!path.empty())
		this->loadModel(path);

	if (!_generateDataset)
	{
		if (!_meshGrid)
			_meshGrid = new RegularGrid(ivec3(_fractParameters._clampVoxelMetricUnit));

		this->allocateMeshGrid(_fractParameters);
	}

	this->rebuildGrid(fractureParameters);
	return this->fractureModel(fractureParameters);
}

void Fragmentation::generateDataset(FragmentationProcedure& fractureProcedure, const std::string& folder, const std::string& extension, const std::string& destinationFolder)
{
	std::vector<std::string> fileList;
	FileManagement::searchFiles(folder, extension, fileList);
	std::vector<FragmentationProcedure::FragmentMetadata> fragmentMetadata;

	fractureProcedure._currentDestinationFolder = destinationFolder;
	if (!std::filesystem::exists(fractureProcedure._currentDestinationFolder)) std::filesystem::create_directory(fractureProcedure._currentDestinationFolder);

	if (!fractureProcedure._startVessel.empty())
	{
		do
		{
			fileList.erase(fileList.begin());
		} 
		while (!fileList.empty() && fileList[0].find(fractureProcedure._startVessel) == std::string::npos);
	}

	this->_generateDataset = true;
	this->allocateMemoryDataset(fractureProcedure);

	for (const std::string& path : fileList)
	{
		std::vector<FragmentationProcedure::FragmentMetadata> modelMetadata;
		this->loadModel(path);

		const std::string modelName = _mesh->getShortName();
		const std::string meshFolder = fractureProcedure._currentDestinationFolder + modelName + "/";
		const std::string meshFile = meshFolder + modelName + "_";
		if (!std::filesystem::exists(meshFolder)) std::filesystem::create_directory(meshFolder);

		// Calculate size of voxelization according to model size
		const AABB aabb = _mesh->getAABB();
		fractureProcedure._fractureParameters._gridSubdivisions = glm::ceil(aabb.size() * vec3(fractureProcedure._fractureParameters._voxelPerMetricUnit));
		if (fractureProcedure._fractureParameters._gridSubdivisions.x > fractureProcedure._fractureParameters._clampVoxelMetricUnit or
			fractureProcedure._fractureParameters._gridSubdivisions.y > fractureProcedure._fractureParameters._clampVoxelMetricUnit or
			fractureProcedure._fractureParameters._gridSubdivisions.z > fractureProcedure._fractureParameters._clampVoxelMetricUnit)
		{
			fractureProcedure._fractureParameters._gridSubdivisions =
				glm::floor(vec3(fractureProcedure._fractureParameters._clampVoxelMetricUnit) *
					aabb.size() / glm::max(aabb.size().x, glm::max(aabb.size().y, aabb.size().z)));
			std::cout << modelName << " - " << "Voxelization size clamped to " << fractureProcedure._fractureParameters._clampVoxelMetricUnit << std::endl;
		}
		while (fractureProcedure._fractureParameters._gridSubdivisions.x % 4 != 0) ++fractureProcedure._fractureParameters._gridSubdivisions.x;
		while (fractureProcedure._fractureParameters._gridSubdivisions.z % 4 != 0) ++fractureProcedure._fractureParameters._gridSubdivisions.z;

		std::cout << modelName << " - " << fractureProcedure._fractureParameters._gridSubdivisions.x << "x" << fractureProcedure._fractureParameters._gridSubdivisions.y << "x" << fractureProcedure._fractureParameters._gridSubdivisions.z << std::endl;

		// Initialize grid content
		_meshGrid->setAABB(_mesh->getAABB(), fractureProcedure._fractureParameters._gridSubdivisions);
		_meshGrid->fill(_mesh->getModelComponent(0));
		_meshGrid->resetMarchingCubes();
		_mesh->getModelComponent(0)->releaseMemory();

		for (int numFragments = fractureProcedure._fragmentInterval.x; numFragments <= fractureProcedure._fragmentInterval.y; ++numFragments)
		{
			const std::string fragmentFile = meshFile + std::to_string(numFragments) + "f_";

			fractureProcedure._fractureParameters._numExtraSeeds = 2;
			fractureProcedure._fractureParameters._numSeeds = numFragments;
			const int numIterations = glm::mix(
				fractureProcedure._iterationInterval.x, fractureProcedure._iterationInterval.y,
				static_cast<float>(numFragments - fractureProcedure._fragmentInterval.x) / (fractureProcedure._fragmentInterval.y - fractureProcedure._fragmentInterval.x));

			std::cout << modelName << " - " << numFragments << " fragments ";
			progressbar bar(numIterations);

			for (int iteration = 0; iteration < numIterations; ++iteration)
			{
				bar.update();

				unsigned idx = 0;
				const std::string itFile = fragmentFile + std::to_string(iteration) + "it";
				std::vector<FragmentationProcedure::FragmentMetadata> localMetadata;

				this->fractureGrid(fragmentMetadata, fractureProcedure._fractureParameters);

				for (Model3D* fracture : _fractureMeshes)
				{
					AssimpModel* cadModel = dynamic_cast<AssimpModel*>(fracture);
					const std::string filename = itFile + "_" + std::to_string(idx);
					std::string simplificationFilename;

					if (!fractureProcedure._fractureParameters._targetTriangles.empty())
					{
						for (int targetCount : fractureProcedure._fractureParameters._targetTriangles)
						{
							simplificationFilename = filename + "_" + std::to_string(targetCount) + fractureProcedure._saveExtension;
							cadModel->simplify(targetCount);
							cadModel->save(simplificationFilename, fractureProcedure._compressFiles);

							fragmentMetadata[idx]._vesselName = simplificationFilename;
							fragmentMetadata[idx]._numVertices = fracture->getNumVertices();
							fragmentMetadata[idx]._numFaces = fracture->getNumFaces();
							localMetadata.push_back(fragmentMetadata[idx]);
						}
					}
					else
					{
						cadModel->save(filename + fractureProcedure._saveExtension, fractureProcedure._compressFiles);

						fragmentMetadata[idx]._vesselName = filename + fractureProcedure._saveExtension;
						fragmentMetadata[idx]._numVertices = fracture->getNumVertices();
						fragmentMetadata[idx]._numFaces = fracture->getNumFaces();
						localMetadata.push_back(fragmentMetadata[idx]);
					}

					++idx;
				}

				modelMetadata.insert(modelMetadata.end(), localMetadata.begin(), localMetadata.end());
				fragmentMetadata.clear();
			}

			std::cout << std::endl;
		}

		if (fractureProcedure._exportMetadata)
		{
			this->exportMetadata(meshFile + "metadata.txt", modelMetadata);
		}

		if (fractureProcedure._compressFiles)
		{
			std::thread compressFolder(&Fragmentation::launchZipingProcess, this, meshFolder);
			compressFolder.detach();
		}
	}
}

// [Protected methods]

void Fragmentation::allocateMemoryDataset(FragmentationProcedure& fractureProcedure)
{
	// Prepare regular grid
	_meshGrid = new RegularGrid(ivec3(fractureProcedure._fractureParameters._clampVoxelMetricUnit));

	// Prepare GPU memory for fracturing
	fracturer::Fracturer* fracturer = fracturer::FloodFracturer::getInstance();

	fractureProcedure._fractureParameters._gridSubdivisions = ivec3(fractureProcedure._fractureParameters._clampVoxelMetricUnit);
	fracturer->prepareSSBOs(&fractureProcedure._fractureParameters);
}

void Fragmentation::allocateMeshGrid(FractureParameters& fractParameters)
{
	const AABB aabb = _mesh->getAABB();
	fractParameters._gridSubdivisions =
		glm::floor(vec3(fractParameters._gridSubdivisions.x) * aabb.size() / glm::max(aabb.size().x, glm::max(aabb.size().y, aabb.size().z)));

	while (fractParameters._gridSubdivisions.x % 4 != 0) ++fractParameters._gridSubdivisions.x;
	while (fractParameters._gridSubdivisions.z % 4 != 0) ++fractParameters._gridSubdivisions.z;

	_meshGrid->setAABB(aabb, fractParameters._gridSubdivisions);
	_meshGrid->fill(_mesh->getModelComponent(0));
	_meshGrid->resetMarchingCubes();
}

void Fragmentation::eraseFragmentContent()
{
	delete _pointCloud;
	_pointCloud = nullptr;

	for (Model3D* fractureMesh : _fractureMeshes) delete fractureMesh;
	_fractureMeshes.clear();
}

void Fragmentation::exportMetadata(const std::string& filename, std::vector<FragmentationProcedure::FragmentMetadata>& fragmentSize)
{
	std::ofstream outputStream(filename.c_str());

	if (outputStream.fail()) return;

	outputStream << "Filename\tFragment id\tVoxelization size\tVoxels\tOccupied voxels\tPercentage\tVertices\tFaces" << std::endl;

	for (int idx = 0; idx < fragmentSize.size(); ++idx)
	{
		outputStream <<
			fragmentSize[idx]._vesselName << "\t" <<
			fragmentSize[idx]._id << "\t" <<
			fragmentSize[idx]._voxelizationSize.x << "x" << fragmentSize[idx]._voxelizationSize.y << "x" << fragmentSize[idx]._voxelizationSize.z << "\t" <<
			fragmentSize[idx]._voxels << "\t" <<
			fragmentSize[idx]._occupiedVoxels << "\t" <<
			fragmentSize[idx]._percentage << "\t" <<
			fragmentSize[idx]._numVertices << "\t" <<
			fragmentSize[idx]._numFaces << std::endl;
	}

	outputStream.close();
}

std::string Fragmentation::fractureModel(FractureParameters& fractParameters)
{
	fracturer::DistanceFunction dfunc = static_cast<fracturer::DistanceFunction>(fractParameters._distanceFunction);
	std::vector<uvec4> seeds;

	if (fractParameters._biasSeeds == 0)
	{
		seeds = fracturer::Seeder::uniform(*_meshGrid, fractParameters._numSeeds, fractParameters._seedingRandom);
	}
	else
	{
		seeds = fracturer::Seeder::uniform(*_meshGrid, fractParameters._numSeeds, fractParameters._seedingRandom);
		seeds = fracturer::Seeder::nearSeeds(*_meshGrid, seeds, fractParameters._biasSeeds - fractParameters._numSeeds, fractParameters._spreading);
	}

	if (fractParameters._numExtraSeeds > 0)
	{
		fracturer::DistanceFunction mergeDFunc = static_cast<fracturer::DistanceFunction>(fractParameters._mergeSeedsDistanceFunction);
		auto extraSeeds = fracturer::Seeder::uniform(*_meshGrid, fractParameters._numExtraSeeds, fractParameters._seedingRandom);
		extraSeeds.insert(extraSeeds.begin(), seeds.begin(), seeds.end());

		fracturer::Seeder::mergeSeeds(seeds, extraSeeds, mergeDFunc);
		seeds = extraSeeds;
	}

	fracturer::Fracturer* fracturer = fracturer::FloodFracturer::getInstance();
	if (!fracturer->setDistanceFunction(dfunc)) return "Invalid distance function";
	fracturer->build(*_meshGrid, seeds, &fractParameters);

	_meshGrid->detectBoundaries(fractParameters._boundarySize);
	if (fractParameters._erode)
	{
		_meshGrid->erode(static_cast<FractureParameters::ErosionType>(
			fractParameters._erosionConvolution), fractParameters._erosionSize, fractParameters._erosionIterations,
			fractParameters._erosionProbability, fractParameters._erosionThreshold);
	}

	_meshGrid->undoMask();

	return "";
}

void Fragmentation::launchZipingProcess(const std::string& folder)
{
	//std::filesystem::copy(folder, destinationFolder, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
	const std::string currentPath = std::filesystem::current_path().string();
	const std::string systemUnit = currentPath.substr(0, currentPath.find_first_of("/\\"));
	const std::string cd = "cd " + currentPath;
	const std::string command = cd + " && " + systemUnit + " && " + "Bash\\zip.bat " + folder + " " + folder.substr(0, folder.find_first_of("/\\")) + " >nul";

	std::system(command.c_str());
}

void Fragmentation::loadModels()
{
	_fragmentMetadata.clear();
	this->fractureGrid(TARGET_PATH, _fragmentMetadata, _fractParameters);
}

void Fragmentation::loadModel(const std::string& path)
{
	delete _mesh;
	_mesh = new AssimpModel(path, true, true, false);
}

void Fragmentation::rebuildGrid(FractureParameters& fractureParameters)
{
	_meshGrid->resetFilling();
}