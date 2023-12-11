#pragma once

#include "stdafx.h"

/**
*	@file FractureParameters.h
*	@authors Alfonso L�pez Ruiz (alr00048@red.ujaen.es)
*	@date 21/07/2021
*/

/**
*	@brief Wraps the rendering conditions of the application.
*/
struct FractureParameters
{
public:
	enum DistanceFunction : uint8_t { EUCLIDEAN, MANHATTAN, CHEBYSHEV, DISTANCE_FUNCTIONS };
	inline static const char* Distance_STR[DISTANCE_FUNCTIONS] = { "Euclidean", "Manhattan", "Chebyshev" };

	enum RandomUniformType { STD_UNIFORM, HALTON, NUM_RANDOM_FUNCTIONS };
	inline static const char* Random_STR[NUM_RANDOM_FUNCTIONS] = { "STD Uniform", "Halton" };

	enum ErosionType { SQUARE, ELLIPSE, CROSS, NUM_EROSION_CONVOLUTIONS };
	inline static const char* Erosion_STR[NUM_EROSION_CONVOLUTIONS] = { "Square", "Ellipse", "Cross" };

	enum NeighbourhoodType { VON_NEUMANN, MOORE, NUM_NEIGHBOURHOODS };
	inline static const char* Neighbourhood_STR[NUM_NEIGHBOURHOODS] = { "Von Neumann", "Moore" };

public:
	int				_biasSeeds;
	int				_boundarySize;
	int				_clampVoxelMetricUnit;
	bool			_computeMCFragments;
	bool			_erode;
	int				_erosionConvolution;
	int				_erosionIterations;
	float			_erosionProbability;
	int				_erosionSize;
	float			_erosionThreshold;
	bool			_fillShape;
	int				_distanceFunction;
	ivec3			_gridSubdivisions;
	bool			_launchGPU;
	int				_marchingCubesSubdivisions;
	int				_mergeSeedsDistanceFunction;
	bool			_metricVoxelization;
	int             _neighbourhoodType;
	int				_numExtraSeeds;
	int				_numSeeds;
	int				_numTriangleSamples;
	int				_pointCloudSeedingRandom;
	bool			_removeIsolatedRegions;
	int				_seed;
	int				_seedingRandom;
	int				_spreading;
	std::vector<int> _targetTriangles;
	int				_voxelPerMetricUnit;

	// Rendering during the build procedure
	bool			_renderGrid;
	bool			_renderMesh;
	bool			_renderPointCloud;

public:
	/**
	*	@brief Default constructor.
	*/
	FractureParameters() :
		_biasSeeds(128),
		_boundarySize(1),
		_clampVoxelMetricUnit(256),
		_computeMCFragments(false),
		_erode(false),
		_erosionConvolution(ELLIPSE),
		_erosionProbability(.5f),
		_erosionIterations(3),
		_erosionSize(3),
		_erosionThreshold(0.5f),
		_fillShape(true),
		_distanceFunction(CHEBYSHEV),
		_gridSubdivisions(256),
		_launchGPU(true),
		_marchingCubesSubdivisions(1),
		_mergeSeedsDistanceFunction(EUCLIDEAN),
		_metricVoxelization(false),
		_neighbourhoodType(VON_NEUMANN),
		_numExtraSeeds(30),
		_numSeeds(8),
		_numTriangleSamples(10000),
		_pointCloudSeedingRandom(STD_UNIFORM),
		_removeIsolatedRegions(true),
		_seed(80),
		_seedingRandom(HALTON),
		_spreading(5),
		_targetTriangles({ 500, 1000 }),
		_voxelPerMetricUnit(90),

		_renderGrid(true),
		_renderMesh(false),
		_renderPointCloud(false)
	{
		std::qsort(_targetTriangles.data(), _targetTriangles.size(), sizeof(int), [](const void* a, const void* b) {
			return *(int*)b - *(int*)a;
			});
	}
};

