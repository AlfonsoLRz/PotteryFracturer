#pragma once

#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Fracturer/Seeder.h"
#include "Geometry/3D/PointCloud3D.h"
#include "Graphics/Core/Model3D.h"

/**
*	@brief Model loaded from an OBJ file.
*/
class AssimpModel : public Model3D
{
public:
	const static std::string BINARY_EXTENSION;					//!< File extension for binary models

protected:
	AABB				_aabb;									//!< Boundaries 
	Assimp::Importer	_assimpImporter;						//!< Assimp importer
	Assimp::Exporter	_assimpExporter;						//!< 
	std::string			_filename;								//!< File path (without extension)
	const aiScene*		_scene;									//!< Scene from assimp library	

protected:
	/**
	*	@brief Fuse components into a single one.
	*/
	void fuseComponents();

	/**
	*	@brief Combines similar vertices.
	*/
	void fuseVertices(std::vector<int>& mapping);

	/**
	*	@brief Fills the content of model component with binary file data.
	*/
	bool loadModelFromBinaryFile(const std::string& binaryFile);

	/**
	*	@brief Processes mesh as loaded by Assimp.
	*/
	Model3D::ModelComponent* processMesh(aiMesh* mesh, const aiScene* scene, const std::string& folder);

	/**
	*	@brief Process node from assimp load.
	*/
	void processNode(aiNode* node, const aiScene* scene, const std::string& folder);

	/**
	*	@brief Loads the CAD model from a binary file, if possible.
	*/
	bool readBinary(const std::string& filename, const std::vector<Model3D::ModelComponent*>& modelComp);

	/**
	*	@brief Remaps vertices to merge them.
	*/
	void remapVertices(Model3D::ModelComponent* modelComponent, std::vector<int>& mapping);

	/**
	*	@brief Saves current model using assimp.
	*/
	bool saveAssimp(const std::string& filename, bool compress = true);

	/**
	*	@brief Saves current model using assimp.
	*/
	void threadedSaveAssimp(aiScene* scene, const std::string& filename, bool zip);

	/**
	*	@brief Writes the model to a binary file in order to fasten the following executions.
	*	@return Success of writing process.
	*/
	bool writeBinary(const std::string& path);

public:
	/**
	*	@brief AssimpModel constructor.
	*	@param filename Path where the model is located.
	*	@param useBinary Use of previously written binary files.
	*/
	AssimpModel(const std::string& filename, const bool useBinary, const bool fuseComponents, const bool fuseVertices);

	/**
	*	@brief Model 3D constructor for a triangle mesh with no known geometry at first.
	*/
	AssimpModel(const mat4& modelMatrix = mat4(1.0f));

	/**
	*	@brief Deleted copy constructor.
	*	@param model Model to copy attributes.
	*/
	AssimpModel(const AssimpModel& model) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~AssimpModel();

	/**
	*	@brief Sends geometry & topology to GPU.
	*/
	void endInsertionBatch(bool releaseMemory = true, bool buildVao = true);

	/**
	*	@return Bounding box of the model.
	*/
	AABB getAABB() { return _aabb; }

	/**
	*	@brief Returns the name of the model.
	*/
	std::string getShortName() const;

	/**
	*	@brief Appends new geometry & topology.
	*/
	void insert(vec4* vertices, unsigned numVertices, uvec4* faces, unsigned numFaces, bool updateIndices = true);

	/**
	*	@brief Deleted assignment operator.
	*	@param model Model to copy attributes.
	*/
	AssimpModel& operator=(const AssimpModel& model) = delete;

	/**
	*	@brief Samples the mesh as a set of points.
	*/
	PointCloud3D* sample(unsigned maxSamples, int randomFunction);

	/**
	*	@brief Saves the model using assimp.
	*/
	bool save(const std::string& filename, bool compress = true);

	/**
	*	@brief
	*/
	void simplify(unsigned numFaces, bool verbose = false);

	/**
	*	@brief Subdivides mesh with the specified maximum area.
	*/
	bool subdivide(float maxArea);
};

