#include "stdafx.h"
#include "AssimpModel.h"

#include "Graphics/Core/ShaderList.h"
#include "quadric-mesh-simplification/Simplify.h"
#include "Utilities/ChronoUtilities.h"

const std::string AssimpModel::BINARY_EXTENSION = ".bin";

/// [Public methods]

AssimpModel::AssimpModel(const std::string& filename, const bool useBinary, const bool fuseComponents, const bool mergeVertices) :
	Model3D(mat4(1.0f), 0), _scene(nullptr), _filename(filename)
{
	std::string binaryFile = _filename.substr(0, _filename.find_last_of('.')) + BINARY_EXTENSION;

	if (useBinary && std::filesystem::exists(binaryFile))
	{
		this->loadModelFromBinaryFile(binaryFile);
	}
	else
	{
		_scene = _assimpImporter.ReadFile(_filename, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenSmoothNormals);
		if (!_scene || _scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !_scene->mRootNode)
			throw std::runtime_error("ERROR::ASSIMP::" + std::string(_assimpImporter.GetErrorString()));

		std::string shortName = _scene->GetShortFilename(_filename.c_str());
		std::string folder = _filename.substr(0, _filename.length() - shortName.length());

		this->processNode(_scene->mRootNode, _scene, folder);
		if (fuseComponents and _modelComp.size() > 1)
			this->fuseComponents();

		if (mergeVertices)
		{
			for (Model3D::ModelComponent* modelComp : _modelComp)
			{
				std::vector<int> mapping(_modelComp[0]->_geometry.size());
				std::iota(mapping.begin(), mapping.end(), 0);

				this->fuseVertices(mapping);
				this->remapVertices(_modelComp[0], mapping);
			}
		}

		std::cout << "Number of vertices: " << this->getNumVertices() << std::endl;
		std::cout << "Number of faces: " << this->getNumFaces() << std::endl;

		this->writeBinary(binaryFile);
	}

	for (ModelComponent* modelComponent : _modelComp)
	{
		modelComponent->releaseMemory(false, false);
		modelComponent->updateSSBO();
	}
}

AssimpModel::AssimpModel(const mat4& modelMatrix) : Model3D(modelMatrix, 1), _scene(nullptr)
{
}

AssimpModel::~AssimpModel()
{
}

void AssimpModel::endInsertionBatch(bool releaseMemory, bool buildVao)
{
	for (ModelComponent* modelComponent : _modelComp)
		modelComponent->releaseMemory(releaseMemory, releaseMemory);
}

std::string AssimpModel::getShortName() const
{
	const size_t barPosition = _filename.find_last_of('/');
	const size_t dotPosition = _filename.find_last_of('.');

	return _filename.substr(barPosition + 1, dotPosition - barPosition - 1);
}

void AssimpModel::insert(vec4* vertices, unsigned numVertices, uvec4* faces, unsigned numFaces, bool updateIndices)
{
	ModelComponent* modelComponent = _modelComp[0];

	unsigned baseGeometryIndex = modelComponent->_geometry.size();
	modelComponent->_geometry.resize(baseGeometryIndex + numVertices);
#pragma omp parallel for
	for (int idx = 0; idx < numVertices; ++idx)
		modelComponent->_geometry[baseGeometryIndex + idx] = Model3D::VertexGPUData{ vec3(vertices[idx].x, vertices[idx].y, vertices[idx].z) };

	unsigned baseTopologyIndex = modelComponent->_topology.size();
	modelComponent->_topology.resize(modelComponent->_topology.size() + numFaces);
#pragma omp parallel for
	for (int idx = 0; idx < numFaces; ++idx)
		modelComponent->_topology[baseTopologyIndex + idx] =
		Model3D::FaceGPUData{
			uvec3(faces[idx].x + baseGeometryIndex, faces[idx].y + baseGeometryIndex, faces[idx].z + baseGeometryIndex) };
}

PointCloud3D* AssimpModel::sample(unsigned maxSamples, int randomFunction)
{
	PointCloud3D* pointCloud = nullptr;

	if (_modelComp.size())
	{
		Model3D::ModelComponent* modelComp = _modelComp[0];
		std::vector<float> noiseBuffer;
		fracturer::Seeder::getFloatNoise(maxSamples, maxSamples * 2, randomFunction, noiseBuffer);

		// Max. triangle area
		float maxArea = FLT_MIN;
		//Triangle3D triangle;
		//for (const Model3D::FaceGPUData& face : modelComp->_topology)
		//{
		//	triangle = Triangle3D(modelComp->_geometry[face._vertices.x]._position, modelComp->_geometry[face._vertices.y]._position, modelComp->_geometry[face._vertices.z]._position);
		//	maxArea = std::max(maxArea, triangle.area());
		//}

		ComputeShader* shader = ShaderList::getInstance()->getComputeShader(ShaderEnum::SAMPLER_SHADER);

		unsigned numPoints = 0, noiseBufferSize = 0;
		unsigned maxPoints = maxSamples * modelComp->_topology.size();

		const GLuint vertexSSBO = ComputeShader::setReadBuffer(modelComp->_geometry, GL_STATIC_DRAW);
		const GLuint faceSSBO = ComputeShader::setReadBuffer(modelComp->_topology, GL_STATIC_DRAW);
		const GLuint pointCloudSSBO = ComputeShader::setWriteBuffer(vec4(), maxPoints, GL_DYNAMIC_DRAW);
		const GLuint countingSSBO = ComputeShader::setReadBuffer(&numPoints, 1, GL_DYNAMIC_DRAW);
		const GLuint noiseSSBO = ComputeShader::setReadBuffer(noiseBuffer, GL_STATIC_DRAW);

		shader->use();
		shader->bindBuffers(std::vector<GLuint> { vertexSSBO, faceSSBO, pointCloudSSBO, countingSSBO, noiseSSBO });
		shader->setUniform("maxArea", maxArea);
		shader->setUniform("numSamples", maxSamples);
		shader->setUniform("numTriangles", static_cast<unsigned>(modelComp->_topology.size()));
		shader->execute(ComputeShader::getNumGroups(maxSamples * modelComp->_topology.size()), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		numPoints = *ComputeShader::readData(countingSSBO, unsigned());
		vec4* pointCloudData = ComputeShader::readData(pointCloudSSBO, vec4());

		pointCloud = new PointCloud3D;
		pointCloud->push_back(pointCloudData, numPoints);

		ComputeShader::deleteBuffers(std::vector<GLuint>{ vertexSSBO, faceSSBO, pointCloudSSBO, countingSSBO, noiseSSBO });
	}

	return pointCloud;
}

bool AssimpModel::save(const std::string& filename, bool compress)
{
	return this->saveAssimp(filename, compress);
}

void AssimpModel::simplify(unsigned numFaces, bool verbose)
{
	for (Model3D::ModelComponent* modelComponent : _modelComp)
	{
		if (modelComponent->_topology.size() > numFaces)
		{
			Simplify::vertices.resize(modelComponent->_geometry.size());
			Simplify::triangles.resize(modelComponent->_topology.size());

#pragma omp parallel for
			for (int i = 0; i < modelComponent->_geometry.size(); ++i)
				Simplify::vertices[i] = Simplify::Vertex{
					vec3f(modelComponent->_geometry[i]._position.x, modelComponent->_geometry[i]._position.y, modelComponent->_geometry[i]._position.z)
			};

#pragma omp parallel for
			for (int i = 0; i < modelComponent->_topology.size(); ++i)
			{
				Simplify::triangles[i] = Simplify::Triangle();
				for (int j = 0; j < 3; ++j)
					Simplify::triangles[i].v[j] = modelComponent->_topology[i]._vertices[j];
			}

			Simplify::simplify_mesh(numFaces, 5.0);

			modelComponent->_geometry.resize(Simplify::vertices.size());
			modelComponent->_topology.resize(Simplify::triangles.size());

#pragma omp parallel for
			for (int i = 0; i < Simplify::vertices.size(); ++i)
				modelComponent->_geometry[i]._position = vec3(Simplify::vertices[i].p.x, Simplify::vertices[i].p.y, Simplify::vertices[i].p.z);

#pragma omp parallel for
			for (int i = 0; i < Simplify::triangles.size(); ++i)
			{
				modelComponent->_topology[i]._vertices = uvec3(Simplify::triangles[i].v[0], Simplify::triangles[i].v[1], Simplify::triangles[i].v[2]);
			}
		}

		if (verbose)
			std::cout << "Simplified to " << modelComponent->_topology.size() << " faces." << std::endl;
	}
}

bool AssimpModel::subdivide(float maxArea)
{
	bool applyChanges = false;
	std::vector<unsigned> faces;

#pragma omp parallel for
	for (int modelCompIdx = 0; modelCompIdx < _modelComp.size(); ++modelCompIdx)
	{
		Model3D::ModelComponent* modelComp = _modelComp[modelCompIdx];

#pragma omp critical
		applyChanges &= modelComp->subdivide(maxArea, faces);
	}

	return applyChanges;
}

/// [Protected methods]

void AssimpModel::fuseComponents()
{
	Model3D::ModelComponent* newModelComponent = new Model3D::ModelComponent();
	unsigned numVertices = 0;

	for (Model3D::ModelComponent* modelComponent : _modelComp)
	{
		newModelComponent->_geometry.insert(newModelComponent->_geometry.end(), modelComponent->_geometry.begin(), modelComponent->_geometry.end());
		for (FaceGPUData& face : modelComponent->_topology)
			face._vertices += numVertices;
		newModelComponent->_topology.insert(newModelComponent->_topology.end(), modelComponent->_topology.begin(), modelComponent->_topology.end());

		numVertices += modelComponent->_geometry.size();

		delete modelComponent;
	}

	_modelComp.clear();
	_modelComp.push_back(newModelComponent);
}

void AssimpModel::fuseVertices(std::vector<int>& mapping)
{
	Model3D::ModelComponent* modelComp = _modelComp[0];

	for (unsigned vertexIdx = 0; vertexIdx < modelComp->_geometry.size(); ++vertexIdx)
	{
		if (mapping[vertexIdx] != vertexIdx)
			continue;

#pragma omp parallel for
		for (int j = vertexIdx + 1; j < modelComp->_geometry.size(); ++j)
		{
			if (mapping[vertexIdx] == vertexIdx && glm::distance(modelComp->_geometry[vertexIdx]._position, modelComp->_geometry[j]._position) < glm::epsilon<float>())
			{
				// Remap
				mapping[j] = vertexIdx;
			}
		}
	}
}

bool AssimpModel::loadModelFromBinaryFile(const std::string& binaryFile)
{
	return this->readBinary(binaryFile, _modelComp);
}

Model3D::ModelComponent* AssimpModel::processMesh(aiMesh* mesh, const aiScene* scene, const std::string& folder)
{
	std::vector<Model3D::VertexGPUData> vertices(mesh->mNumVertices);
	std::vector<Model3D::FaceGPUData> faces(mesh->mNumFaces);
	AABB aabb;
	aiMaterial* aiMaterial = nullptr;

	// Vertices
	int numVertices = static_cast<int>(mesh->mNumVertices);

#pragma omp parallel for
	for (int i = 0; i < numVertices; i++)
	{
		Model3D::VertexGPUData vertex;
		vertex._position = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
		vertex._normal = vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

		if (mesh->mTangents) vertex._tangent = vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
		if (mesh->mTextureCoords[0]) vertex._textCoord = vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

		vertices[i] = vertex;

#pragma omp critical
		aabb.update(vertex._position);
	}

	// Indices
#pragma omp parallel for
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace& face = mesh->mFaces[i];
		faces[i] = Model3D::FaceGPUData{ uvec3(face.mIndices[0], face.mIndices[1], face.mIndices[2]) };
	}

	ModelComponent* component = new ModelComponent;
	component->_geometry = std::move(vertices);
	component->_topology = std::move(faces);
	component->_aabb = std::move(aabb);

	return component;
}

void AssimpModel::processNode(aiNode* node, const aiScene* scene, const std::string& folder)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		_modelComp.push_back(this->processMesh(mesh, scene, folder));
		_aabb.update(_modelComp[_modelComp.size() - 1]->_aabb);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		this->processNode(node->mChildren[i], scene, folder);
	}
}

bool AssimpModel::readBinary(const std::string& filename, const std::vector<Model3D::ModelComponent*>& modelComp)
{
	std::ifstream fin(filename, std::ios::in | std::ios::binary);
	if (!fin.is_open()) return false;

	size_t numModelComps, numVertices, numTriangles, numIndices;

	fin.read((char*)&numModelComps, sizeof(size_t));
	while (_modelComp.size() < numModelComps)
	{
		_modelComp.push_back(new ModelComponent());
	}

	for (Model3D::ModelComponent* component : modelComp)
	{
		fin.read((char*)&numVertices, sizeof(size_t));
		component->_geometry.resize(numVertices);
		if (numVertices)
			fin.read((char*)&component->_geometry[0], numVertices * sizeof(Model3D::VertexGPUData));

		fin.read((char*)&numTriangles, sizeof(size_t));
		component->_topology.resize(numTriangles);
		if (numTriangles)
			fin.read((char*)&component->_topology[0], numTriangles * sizeof(Model3D::FaceGPUData));

		fin.read((char*)&component->_aabb, sizeof(AABB));
	}

	fin.read((char*)&_aabb, sizeof(AABB));
	fin.close();

	return true;
}

void AssimpModel::remapVertices(Model3D::ModelComponent* modelComponent, std::vector<int>& mapping)
{
	unsigned erasedVertices = 0, startingSize = modelComponent->_geometry.size();
	std::vector<unsigned> newMapping(startingSize);

	for (size_t vertexIdx = 0; vertexIdx < startingSize; ++vertexIdx)
	{
		newMapping[vertexIdx] = vertexIdx - erasedVertices;

		if (mapping[vertexIdx] != vertexIdx)
		{
			modelComponent->_geometry.erase(modelComponent->_geometry.begin() + vertexIdx - erasedVertices);
			++erasedVertices;
		}
	}

	for (FaceGPUData& face : modelComponent->_topology)
	{
		for (int i = 0; i < 3; ++i)
		{
			face._vertices[i] = newMapping[mapping[face._vertices[i]]];
		}
	}
}

bool AssimpModel::saveAssimp(const std::string& filename, bool compress)
{
	aiScene* scene = new aiScene;
	scene->mRootNode = new aiNode();

	scene->mMaterials = new aiMaterial * [1];
	scene->mMaterials[0] = nullptr;
	scene->mNumMaterials = 1;

	scene->mMaterials[0] = new aiMaterial();

	scene->mMeshes = new aiMesh * [1];
	scene->mMeshes[0] = nullptr;
	scene->mNumMeshes = 1;

	scene->mMeshes[0] = new aiMesh();
	scene->mMeshes[0]->mMaterialIndex = 0;

	scene->mRootNode->mMeshes = new unsigned int[1];
	scene->mRootNode->mMeshes[0] = 0;
	scene->mRootNode->mNumMeshes = 1;

	auto pMesh = scene->mMeshes[0];

	// Retrieving info from the model
	glm::uint geometrySize = 0;
	std::vector<glm::vec3> vertices;
	std::vector<ivec3> faces;

	for (ModelComponent* modelComponent : _modelComp)
	{
		for (VertexGPUData& vertex : modelComponent->_geometry)
			vertices.push_back(vertex._position);

		for (FaceGPUData& face : modelComponent->_topology)
			faces.push_back(ivec3(face._vertices.x + geometrySize, face._vertices.y + geometrySize, face._vertices.z + geometrySize));

		geometrySize += modelComponent->_geometry.size();
	}

	// Vertex generation
	const auto& assimpVertices = vertices;

	pMesh->mVertices = new aiVector3D[assimpVertices.size()];
	pMesh->mNumVertices = assimpVertices.size();

	int j = 0;
	for (auto itr = assimpVertices.begin(); itr != assimpVertices.end(); ++itr)
	{
		pMesh->mVertices[itr - assimpVertices.begin()] = aiVector3D(assimpVertices[j].x, assimpVertices[j].y, assimpVertices[j].z);
		++j;
	}

	// Index generation
	pMesh->mFaces = new aiFace[faces.size()];
	pMesh->mNumFaces = static_cast<unsigned>(faces.size());

	for (size_t i = 0; i < faces.size(); i++)
	{
		aiFace& face = pMesh->mFaces[i];
		face.mIndices = new unsigned int[3];
		face.mNumIndices = 3;
		face.mIndices[0] = faces[i].x;
		face.mIndices[1] = faces[i].y;
		face.mIndices[2] = faces[i].z;
	}

	// Out of main thread
	std::thread writeModel(&AssimpModel::threadedSaveAssimp, this, scene, filename, compress);
	writeModel.detach();

	return true;
}

void AssimpModel::threadedSaveAssimp(aiScene* scene, const std::string& filename, bool zip)
{
	const std::string file = filename.substr(filename.find_last_of('/') + 1);
	Assimp::Exporter exporter;
	exporter.Export(scene, "stl", filename);
	delete scene;
}

bool AssimpModel::writeBinary(const std::string& path)
{
	std::ofstream fout(path, std::ios::out | std::ios::binary);
	if (!fout.is_open())
	{
		return false;
	}

	size_t numIndices;
	const size_t numModelComps = _modelComp.size();
	fout.write((char*)&numModelComps, sizeof(size_t));

	for (Model3D::ModelComponent* component : _modelComp)
	{
		const size_t numVertices = component->_geometry.size();
		fout.write((char*)&numVertices, sizeof(size_t));
		fout.write((char*)&component->_geometry[0], numVertices * sizeof(Model3D::VertexGPUData));

		const size_t numTriangles = component->_topology.size();
		fout.write((char*)&numTriangles, sizeof(size_t));
		fout.write((char*)&component->_topology[0], numTriangles * sizeof(Model3D::FaceGPUData));

		fout.write((char*)(&component->_aabb), sizeof(AABB));
	}

	fout.write((char*)&_aabb, sizeof(AABB));

	fout.close();

	return true;
}