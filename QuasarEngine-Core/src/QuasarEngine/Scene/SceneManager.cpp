#include "qepch.h"
#include "SceneManager.h"

#include <iostream>
#include <filesystem>
#include <fstream>

#include <QuasarEngine.h>

#include "SceneSerializer.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Tools/Utils.h>
#include <QuasarEngine/Entity/Components/CameraComponent.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>
#include <QuasarEngine/Entity/Components/MaterialComponent.h>
#include <QuasarEngine/Entity/Components/MaterialComponent.h>
#include <QuasarEngine/Entity/Components/LightComponent.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>
#include <QuasarEngine/Entity/Components/MeshRendererComponent.h>

#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Entity/Components/Animation/AnimationComponent.h>

#include <QuasarEngine/Scene/Importer/TextureConfigImporter.h>

namespace QuasarEngine
{
	SceneManager::SceneManager(std::filesystem::path assetPath) : m_AssetPath(assetPath)
	{
		m_SceneObject = std::make_unique<SceneObject>();
	}

	SceneManager::~SceneManager()
	{
		m_SceneObject->GetScene().ClearEntities();
	}

	void SceneManager::Update(double dt)
	{
		if (m_SceneObject)
		{
			m_SceneObject->GetScene().Update(dt);

			if (m_PendingAction != PendingAction::None)
			{
				if (m_SceneObject->GetScene().IsEmpty())
				{
					switch (m_PendingAction)
					{
					case PendingAction::LoadScene:
					{
						SceneSerializer serializer(*m_SceneObject, m_AssetPath);
						bool result = serializer.Deserialize(m_PendingActionFile);
						break;
					}
					case PendingAction::LoadSceneWithPath:
					{
						m_SceneObject->CreateScene();
						Renderer::Instance().BeginScene(m_SceneObject->GetScene());
						SceneSerializer serializer(*m_SceneObject, m_AssetPath);
						bool result = serializer.Deserialize(m_PendingActionFile);
						std::cout << "Scene loaded from path: " << m_PendingActionFile << "\n";
						break;
					}
					case PendingAction::ReloadScene:
					{
						SceneSerializer serializer(*m_SceneObject, m_AssetPath);
						bool result = serializer.Deserialize(m_PendingActionFile);
						break;
					}
					case PendingAction::CreateNewScene:
					{
						m_SceneObject->CreateScene();
						break;
					}
					default: break;
					}

					m_PendingAction = PendingAction::None;
					m_PendingActionFile.clear();
				}
			}

			while (!m_ModelsToLoad.empty())
			{
				ModelToLoad modelReq = m_ModelsToLoad.front();
				m_ModelsToLoad.pop();

				std::filesystem::path absPath;
				try {
					absPath = std::filesystem::weakly_canonical(modelReq.path);
				}
				catch (...) {
					absPath = std::filesystem::absolute(modelReq.path);
				}
				absPath = absPath.lexically_normal();

				const std::filesystem::path assetsRoot = (m_AssetPath / "Assets").lexically_normal();

				std::error_code ec{};
				std::string id;
				if (auto rel = std::filesystem::relative(absPath, assetsRoot, ec); !ec && !rel.empty())
					id = "Assets/" + rel.generic_string();
				else
					id = "Assets/" + absPath.filename().generic_string();

				const std::string full = absPath.generic_string();

				if (!AssetManager::Instance().isAssetLoaded(id))
				{
					QuasarEngine::AssetToLoad toLoad{};
					toLoad.id = id;
					toLoad.path = full;
					toLoad.type = QuasarEngine::AssetType::MODEL;

					AssetManager::Instance().loadAsset(toLoad);

					m_ModelsToLoad.push(modelReq);
					break;
				}

				std::shared_ptr<Model> modelPtr = AssetManager::Instance().getAsset<Model>(id);
				if (!modelPtr)
				{
					m_ModelsToLoad.push(modelReq);
					break;
				}

				const std::string fileName = std::filesystem::path(id).stem().string();
				Entity rootEntity = GetActiveScene().CreateEntity(fileName);

				std::unordered_map<std::string, Entity> nodeEntities;

				nodeEntities[""] = rootEntity;

				auto clips = QuasarEngine::LoadAnimationClips(full);
				if (!clips.empty())
				{
					auto& anim = rootEntity.AddComponent<QuasarEngine::AnimationComponent>(id);
					anim.SetClips(std::move(clips));
					anim.Play(0, true, 1.0f);
				}

				auto NormalizeNodePath = [](const std::string& path) -> std::string
					{
						std::string result;
						size_t start = 0;
						while (start < path.size())
						{
							size_t slash = path.find('/', start);
							std::string seg = (slash == std::string::npos)
								? path.substr(start)
								: path.substr(start, slash - start);

							if (!seg.empty() && seg != "ROOT")
							{
								if (!result.empty())
									result += '/';
								result += seg;
							}

							if (slash == std::string::npos)
								break;
							start = slash + 1;
						}
						return result;
					};

				std::function<Entity(const std::string&)> GetOrCreateNodeEntity =
					[&](const std::string& normalizedPath) -> Entity
					{
						if (normalizedPath.empty())
							return rootEntity;

						if (auto it = nodeEntities.find(normalizedPath); it != nodeEntities.end())
							return it->second;

						size_t slash = normalizedPath.rfind('/');
						std::string parentPath;
						std::string nodeName;
						if (slash == std::string::npos)
						{
							parentPath = "";
							nodeName = normalizedPath;
						}
						else
						{
							parentPath = normalizedPath.substr(0, slash);
							nodeName = normalizedPath.substr(slash + 1);
						}

						Entity parentEntity = GetOrCreateNodeEntity(parentPath);

						Entity nodeEntity = GetActiveScene().CreateEntity(nodeName);
						parentEntity.GetComponent<HierarchyComponent>()
							.AddChild(parentEntity.GetUUID(), nodeEntity.GetUUID());

						nodeEntities[normalizedPath] = nodeEntity;
						return nodeEntity;
					};

				auto fillTexSpec = [&](const std::optional<std::string>& texId,
					std::optional<TextureSpecification>& outSpec)
					{
						if (!texId || texId->empty()) return;
						const std::filesystem::path texPath = AssetManager::Instance().ResolvePath(*texId);
						TextureSpecification spec = TextureConfigImporter::ImportTextureConfig(texPath.generic_string());
						outSpec = spec;
					};

				modelPtr->ForEachInstance(
					[&](const MeshInstance& inst, const glm::mat4& nodeLocal, const std::string& nodePath)
					{
						const std::string logicalPath = NormalizeNodePath(nodePath);

						Entity nodeEntity = GetOrCreateNodeEntity(logicalPath);

						Entity meshEntity = GetActiveScene().CreateEntity(inst.name);
						meshEntity.AddComponent<MeshRendererComponent>();

						auto& mc = meshEntity.AddComponent<MeshComponent>(inst.name);
						mc.SetMesh(inst.mesh.get());
						mc.SetModelPath(full);
						mc.SetNodePath(nodePath);
						mc.SetLocalNodeTransform(nodeLocal);

						MaterialSpecification matSpec = inst.material;
						fillTexSpec(matSpec.AlbedoTexture, matSpec.AlbedoTextureSpec);
						fillTexSpec(matSpec.NormalTexture, matSpec.NormalTextureSpec);
						fillTexSpec(matSpec.MetallicTexture, matSpec.MetallicTextureSpec);
						fillTexSpec(matSpec.RoughnessTexture, matSpec.RoughnessTextureSpec);
						fillTexSpec(matSpec.AOTexture, matSpec.AOTextureSpec);

						meshEntity.AddComponent<MaterialComponent>(matSpec);

						nodeEntity.GetComponent<HierarchyComponent>()
							.AddChild(nodeEntity.GetUUID(), meshEntity.GetUUID());
					}
				);
			}
		}
	}

	void SceneManager::LoadModel(const ModelToLoad& modelToLoad)
	{
		m_ModelsToLoad.push(modelToLoad);
	}

	void SceneManager::AddGameObject(const std::string& file)
	{
		LoadModel({ file });
	}

	void SceneManager::AddCube()
	{
		LoadModel({ "Assets/Models/cube.obj" });
	}

	void SceneManager::AddSphere()
	{
		LoadModel({ "Assets/Models/sphere.obj" });
	}

	void SceneManager::AddUVSphere()
	{
		Entity temp = m_SceneObject->GetScene().CreateEntity("UV Sphere");

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;

		const float PI = 3.14159265359f;

		bool oddRow = false;

		vertices.reserve((X_SEGMENTS + 1) * (Y_SEGMENTS + 1) * 11);

		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;

				float theta = xSegment * 2.0f * PI;
				float phi = ySegment * PI;

				float sinPhi = std::sin(phi);
				float cosPhi = std::cos(phi);
				float sinTheta = std::sin(theta);
				float cosTheta = std::cos(theta);

				float xPos = cosTheta * sinPhi;
				float yPos = cosPhi;
				float zPos = sinTheta * sinPhi;

				float nx = xPos;
				float ny = yPos;
				float nz = zPos;

				float u = xSegment;
				float v = ySegment;

				float tx = -sinTheta * sinPhi;
				float ty = 0.0f;
				float tz = cosTheta * sinPhi;

				if (sinPhi < 0.0001f)
				{
					tx = 1.0f;
					ty = 0.0f;
					tz = 0.0f;
				}

				vertices.push_back(xPos);
				vertices.push_back(yPos);
				vertices.push_back(zPos);

				vertices.push_back(nx);
				vertices.push_back(ny);
				vertices.push_back(nz);

				vertices.push_back(u);
				vertices.push_back(v);

				vertices.push_back(tx);
				vertices.push_back(ty);
				vertices.push_back(tz);
			}
		}

		for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow)
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
				{
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}

		temp.AddComponent<MaterialComponent>();
		temp.AddComponent<MeshRendererComponent>();
		temp.AddComponent<MeshComponent>().GenerateMesh(
			vertices,
			indices,
			{},
			DrawMode::TRIANGLE_STRIP
		);
	}

	void SceneManager::AddPlane()
	{
		LoadModel({ "Assets/Models/plane.obj" });
	}

	void SceneManager::SaveScene()
	{
		if (m_SceneObject->GetPath() != "")
		{
			SceneSerializer serializer(*m_SceneObject, m_AssetPath);
			serializer.Serialize(m_SceneObject->GetPath());
		}
		else
		{
			std::optional<Utils::FileInfo> fileInfos = Utils::saveFile();
			if (fileInfos.has_value())
			{
				m_SceneObject->SetName(fileInfos.value().fileName);
				std::cout << fileInfos.value().filePath << std::endl;
				std::cout << fileInfos.value().selectedFile << std::endl;
				SceneSerializer serializer(*m_SceneObject, m_AssetPath);
				serializer.Serialize(fileInfos.value().filePath);
			}
		}
	}

	void SceneManager::LoadScene()
	{
		std::optional<Utils::FileInfo> fileInfos = Utils::openFile();
		if (fileInfos.has_value())
		{
			m_SceneObject->GetScene().ClearEntities();
			m_PendingAction = PendingAction::LoadScene;
			m_PendingActionFile = fileInfos.value().filePath;
		}
	}

	void SceneManager::LoadScene(std::string filePath)
	{
		m_SceneObject->GetScene().ClearEntities();
		m_PendingAction = PendingAction::LoadSceneWithPath;
		m_PendingActionFile = filePath;
	}

	void SceneManager::ReloadScene(std::string filePath)
	{
		m_SceneObject->GetScene().ClearEntities();
		//m_SceneObject->CreateScene();
		m_PendingAction = PendingAction::ReloadScene;
		m_PendingActionFile = filePath;
	}

	void SceneManager::createNewScene()
	{
		m_SceneObject->GetScene().ClearEntities();
		m_PendingAction = PendingAction::CreateNewScene;
	}

	void SceneManager::OpenExternalFile()
	{
		std::optional<Utils::FileInfo> fileInfos = Utils::openFile();
		if (fileInfos.has_value())
		{
			std::filesystem::path sourceFile = fileInfos.value().filePath;
			std::filesystem::path targetParent = "Assets";
			auto target = targetParent / sourceFile.filename();

			try
			{
				std::filesystem::create_directories(targetParent);
				std::filesystem::copy_file(sourceFile, target, std::filesystem::copy_options::overwrite_existing);
			}
			catch (std::exception& e)
			{
				std::cout << e.what();
			}
		}
	}
}