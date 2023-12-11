#pragma once

#include "Geometry/3D/AABB.h"
#include "Graphics/Core/ColorUtilities.h"
#include "Graphics/Core/GraphicsCoreEnumerations.h"
#include "Graphics/Core/ShaderProgram.h"
#include "Utilities/RandomUtilities.h"

/**
*	@brief Base class for any drawable 3D model.
*/
class Model3D
{
public:
	class ModelComponent;
	friend class Octree;

public:
	/**
	*	@brief Struct which wraps all that information for a mesh vertex.
	*/
	struct VertexGPUData
	{
		vec3		_position;
		float		_padding1;

		vec3		_normal;
		float		_padding2;

		vec2		_textCoord;
		vec2		_padding3;

		vec3		_tangent;
		float		_padding4;
	};

	/**
	*	@brief Struct which wraps a mesh data.
	*/
	struct FaceGPUData
	{
		uvec3		_vertices;
		unsigned	_modelCompID;							//!< ID of model component where the face belongs to

		vec3		_minPoint;								//!< Bounding box corner
		float		_padding1;

		vec3		_maxPoint;
		float		_padding2;

		vec3		_normal;								//!< Accelerates LiDAR intersections 
		float		_padding3;
	};

protected:
	bool							_loaded;				//!< Mark to know if model has already been loaded
	std::vector<ModelComponent*>	_modelComp;				//!< One for each part of the mode, i.e. a revolution object with two bases and a body
	glm::mat4						_modelMatrix;			//!< World transformation

public:
	/**
	*	@brief Model 3D constructor.
	*	@param modelMatrix Transformation of model in world.
	*	@param numComponents Number of component which compose the model.
	*/
	Model3D(const mat4& modelMatrix = mat4(1.0f), unsigned numComponents = 1);

	/**
	*	@brief Copy constructor.
	*/
	Model3D(const Model3D& model) = delete;

	/**
	*	@brief Destructor.
	*/
	virtual ~Model3D();

	/**
	*	@brief Assignment operator overriding.
	*/
	Model3D& operator=(const Model3D& model) = delete;

	// ------------- Getters ----------------

	/**
	*	@return Model component located at the specified index.
	*/
	ModelComponent* getModelComponent(unsigned index) { return _modelComp[index]; }

	/**
	*	@return Model component located at the specified index.
	*/
	std::vector<ModelComponent*> getModelComponents() { return _modelComp; }

	/**
	*	@return Model transformation matrix.
	*/
	mat4 getModelMatrix() { return _modelMatrix; }

	/**
	*	@brief Returns the number of faces comprised in this model.
	*/
	unsigned getNumFaces();

	/**
	*	@return Number of vertices comprised in this model.
	*/
	unsigned getNumVertices();

	// -------------- Setters ------------------

	/**
	*	@brief Assigns a new matrix for model transformation.
	*/
	void setModelMatrix(const mat4& modelMatrix) { _modelMatrix = modelMatrix; }
};

class Model3D::ModelComponent
{
public:
	unsigned						_id;										//!<

	// [GPU Data]
	std::vector<VertexGPUData>		_geometry;									//!<
	std::vector<FaceGPUData>		_topology;									//!<
	GLuint							_geometrySSBO;								//!<
	GLuint							_topologySSBO;
			
	// [Additional info]
	AABB							_aabb;										//!<		

public:
	/**
	*	@brief Default constructor.
	*	@param root Model where this component is located.
	*/
	ModelComponent();
	
	/**
	*	@brief Deleted copy constructor.
	*/
	ModelComponent(const ModelComponent& modelComp) = delete;

	/**
	*	@brief Destructor.
	*/
	~ModelComponent();

	/**
	*	@brief Assignment operator overriding.
	*/
	ModelComponent& operator=(const ModelComponent& orig) = delete;

	/**
	*	@brief Clear geometry and topology arrays to free memory linked to process.
	*/
	virtual void releaseMemory(bool geometry = true, bool topology = true);

	/**
	*	@brief Subdivides the faces to fit the maximum wanted area.
	*/
	bool subdivide(float maxArea, std::vector<unsigned>& maskFaces);

	/**
	*	@brief Updates geometry and topology SSBOs.
	*/
	void updateSSBO();
};