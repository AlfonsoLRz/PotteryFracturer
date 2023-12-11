#include "stdafx.h"
#include "Model3D.h"

#include "Graphics/Core/ShaderList.h"
#include "Utilities/ChronoUtilities.h"

/// [Public methods]

Model3D::Model3D(const glm::mat4& modelMatrix, unsigned numComponents) :
	_loaded(false), _modelMatrix(modelMatrix), _modelComp(numComponents)
{
	for (int i = 0; i < numComponents; ++i)
		_modelComp[i] = new ModelComponent();
}

Model3D::~Model3D()
{
	for (auto& it : _modelComp)
		delete it;
}

unsigned Model3D::getNumFaces()
{
	unsigned numFaces = 0;

	for (ModelComponent* modelComp : _modelComp)
		numFaces += modelComp->_topology.size();

	return numFaces;
}

unsigned Model3D::getNumVertices()
{
	unsigned numVertices = 0;

	for (ModelComponent* modelComp : _modelComp)
		numVertices += modelComp->_geometry.size();

	return numVertices;
}

/// [Public methods]

Model3D::ModelComponent::ModelComponent() :
	_id(-1), _geometrySSBO(UINT_MAX), _topologySSBO(UINT_MAX)
{
}

Model3D::ModelComponent::~ModelComponent()
{
	this->releaseMemory(true, true);
}

void Model3D::ModelComponent::releaseMemory(bool geometry, bool topology)
{
	if (geometry) std::vector<VertexGPUData>().swap(_geometry);
	if (topology) std::vector<FaceGPUData>().swap(_topology);

	if (_geometrySSBO != UINT_MAX) ComputeShader::deleteBuffer(_geometrySSBO);
	if (_topologySSBO != UINT_MAX) ComputeShader::deleteBuffer(_topologySSBO);
}

bool Model3D::ModelComponent::subdivide(float maxArea, std::vector<unsigned>& maskFaces)
{
	bool applyChangesComp = false;

	//auto subdivideTriangle = [=](int faceIdx) -> bool {
	//	bool applyChanges = false;
	//	Triangle3D triangle(this->_geometry[this->_topology[faceIdx]._vertices.x]._position,
	//		this->_geometry[this->_topology[faceIdx]._vertices.y]._position,
	//		this->_geometry[this->_topology[faceIdx]._vertices.z]._position);

	//	if (triangle.area() > maxArea)
	//	{
	//		// Subdivide
	//		triangle.subdivide(this->_geometry, this->_topology, this->_topology[faceIdx], maxArea);
	//		applyChanges = true;
	//		this->_topology.erase(this->_topology.begin() + faceIdx);
	//		--faceIdx;
	//	}

	//	return applyChanges;
	//	};

	//if (maskFaces.empty())
	//{
	//	unsigned numFaces = this->_topology.size();

	//	for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx)
	//	{
	//		if (subdivideTriangle(faceIdx))
	//		{
	//			applyChangesComp = true;
	//			--faceIdx;
	//		}
	//	}
	//}
	//else
	//{
	//	unsigned numFaces = maskFaces.size(), numErasedFaces = 0;

	//	for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx)
	//	{
	//		if (subdivideTriangle(maskFaces[faceIdx] - numErasedFaces))
	//		{
	//			applyChangesComp = true;
	//			--faceIdx;
	//			++numErasedFaces;
	//		}
	//	}
	//}

	return applyChangesComp;
}

void Model3D::ModelComponent::updateSSBO()
{
	if (_geometrySSBO == UINT_MAX)
		_geometrySSBO = ComputeShader::setReadBuffer(_geometry, GL_STATIC_DRAW);
	else
		ComputeShader::updateReadBuffer(_geometrySSBO, _geometry.data(), _geometry.size(), GL_STATIC_DRAW);

	if (_topologySSBO == UINT_MAX)
		_topologySSBO = ComputeShader::setReadBuffer(_topology, GL_STATIC_DRAW);
	else
		ComputeShader::updateReadBuffer(_topologySSBO, _topology.data(), _topology.size(), GL_STATIC_DRAW);
}