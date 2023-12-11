#include "stdafx.h"
#include "ShaderList.h"

// [Static members initialization]

std::unordered_map<uint8_t, std::string> ShaderList::COMP_SHADER_SOURCE 
{
		{ShaderEnum::ASSIGN_FACE_CLUSTER, "Assets/Shaders/Compute/Fracturer/assignFaceCluster"},
		{ShaderEnum::ASSIGN_VERTEX_CLUSTER, "Assets/Shaders/Compute/Fracturer/assignVertexCluster"},
		{ShaderEnum::BIT_MASK_RADIX_SORT, "Assets/Shaders/Compute/RadixSort/bitMask-radixSort"},
		{ShaderEnum::BUILD_MARCHING_CUBES_FACES, "Assets/Shaders/Compute/Fracturer/buildMarchingCubesFaces"},
		{ShaderEnum::BUILD_REGULAR_GRID, "Assets/Shaders/Compute/Fracturer/buildRegularGrid"},
		{ShaderEnum::COMPUTE_MORTON_CODES_FRACTURER, "Assets/Shaders/Compute/Fracturer/computeMortonCodes"},
		{ShaderEnum::COUNT_QUADRANT_OCCUPANCY, "Assets/Shaders/Compute/Fracturer/countQuadrantOccupancy"},
		{ShaderEnum::COUNT_VOXEL_TRIANGLE, "Assets/Shaders/Compute/Fracturer/countVoxelTriangle"},
		{ShaderEnum::DETECT_BOUNDARIES, "Assets/Shaders/Compute/Fracturer/detectBoundaries"},
		{ShaderEnum::DOWN_SWEEP_PREFIX_SCAN, "Assets/Shaders/Compute/PrefixScan/downSweep-prefixScan"},
		{ShaderEnum::END_LOOP_COMPUTATIONS, "Assets/Shaders/Compute/BVHGeneration/endLoopComputations"},
		{ShaderEnum::ERODE_GRID, "Assets/Shaders/Compute/Fracturer/erodeGrid"},
		{ShaderEnum::FILL_REGULAR_GRID, "Assets/Shaders/Compute/Collision/fillRegularGrid"},
		{ShaderEnum::FILL_REGULAR_GRID_VOXEL, "Assets/Shaders/Compute/Fracturer/fillRegularGrid"},
		{ShaderEnum::FINISH_FILL, "Assets/Shaders/Compute/Fracturer/finishFill"},
		{ShaderEnum::FINISH_LAPLACIAN_SMOOTHING, "Assets/Shaders/Compute/Fracturer/finishLaplacianSmoothing"},
		{ShaderEnum::FLOOD_FRACTURER, "Assets/Shaders/Compute/Fracturer/floodFracturer"},
		{ShaderEnum::FUSE_VERTICES_01, "Assets/Shaders/Compute/Fracturer/findSameVertices_01"},
		{ShaderEnum::FUSE_VERTICES_02, "Assets/Shaders/Compute/Fracturer/findSameVertices_02"},
		{ShaderEnum::LAPLACIAN_SMOOTHING, "Assets/Shaders/Compute/Fracturer/laplacianSmoothing"},
		{ShaderEnum::MARCHING_CUBES, "Assets/Shaders/Compute/Fracturer/marchingCubes"},
		{ShaderEnum::MARK_BOUNDARY_TRIANGLES, "Assets/Shaders/Compute/Fracturer/markBoundaryTriangles"},
		{ShaderEnum::NAIVE_FRACTURER, "Assets/Shaders/Compute/Fracturer/naiveFracturer"},
		{ShaderEnum::REALLOCATE_RADIX_SORT, "Assets/Shaders/Compute/RadixSort/reallocateIndices-radixSort"},
		{ShaderEnum::REDUCE_PREFIX_SCAN, "Assets/Shaders/Compute/PrefixScan/reduce-prefixScan"},
		{ShaderEnum::REMOVE_ISOLATED_REGIONS, "Assets/Shaders/Compute/Fracturer/removeIsolatedRegions"},
		{ShaderEnum::RESET_BUFFER_INDEX, "Assets/Shaders/Compute/Generic/resetBufferIndex"},
		{ShaderEnum::RESET_BUFFER, "Assets/Shaders/Compute/Fracturer/resetBuffer"},
		{ShaderEnum::RESET_LAPLACIAN_SMOOTHING, "Assets/Shaders/Compute/Fracturer/resetLaplacianBuffer"},
		{ShaderEnum::RESET_LAST_POSITION_PREFIX_SCAN, "Assets/Shaders/Compute/PrefixScan/resetLastPosition-prefixScan"},
		{ShaderEnum::SAMPLER_SHADER, "Assets/Shaders/Compute/Model/sampler"},
		{ShaderEnum::SELECT_VOXEL_TRIANGLE, "Assets/Shaders/Compute/Fracturer/selectVoxelTriangle"},
		{ShaderEnum::UNDO_MASK_SHADER, "Assets/Shaders/Compute/Fracturer/undoMask"}
};

std::vector<std::unique_ptr<ComputeShader>> ShaderList::_computeShader (ShaderEnum::NUM_COMPUTE_SHADERS);

/// [Protected methods]

ShaderList::ShaderList()
{
}

/// [Public methods]

ComputeShader* ShaderList::getComputeShader(const ShaderEnum::ComputeShaderType shader)
{
	const int shaderID = shader;

	if (!_computeShader[shader].get())
	{
		ComputeShader* shader = new ComputeShader();
		shader->createShaderProgram(COMP_SHADER_SOURCE.at(shaderID).c_str());

		_computeShader[shaderID].reset(shader);
	}

	return _computeShader[shaderID].get();
}