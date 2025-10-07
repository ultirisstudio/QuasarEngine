#include "SceneManager.h"

#include <iostream>
#include <filesystem>
#include <fstream>

#include <QuasarEngine.h>

#include "SceneSerializer.h"
#include "Tools/Utils.h"
#include "Editor/Importer/TextureConfigImporter.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/CameraComponent.h>
#include <QuasarEngine/Entity/Components/MeshComponent.h>
#include <QuasarEngine/Entity/Components/MaterialComponent.h>
#include <QuasarEngine/Entity/Components/MaterialComponent.h>
#include <QuasarEngine/Entity/Components/LightComponent.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>
#include <QuasarEngine/Entity/Components/MeshRendererComponent.h>

#include <QuasarEngine/Physic/PhysicEngine.h>

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
				ModelToLoad model = m_ModelsToLoad.front();
				m_ModelsToLoad.pop();

				std::string id = std::filesystem::path(model.path).filename().string();
				std::filesystem::path full = AssetManager::Instance().ResolvePath(id);

				if (!AssetManager::Instance().isAssetLoaded(id))
				{
					QuasarEngine::AssetToLoad toLoad{};
					toLoad.id = id;
					toLoad.path = full.generic_string();
					toLoad.type = QuasarEngine::AssetType::MODEL;

					AssetManager::Instance().loadAsset(toLoad);

					m_ModelsToLoad.push(model);
					continue;
				}

				std::shared_ptr<Model> modelPtr = AssetManager::Instance().getAsset<Model>(id);
				if (!modelPtr)
				{
					m_ModelsToLoad.push(model);
					break;
				}

				const std::string fileName = std::filesystem::path(id).stem().string();
				Entity entity = GetActiveScene().CreateEntity(fileName);

				auto fillTexSpec = [&](const std::optional<std::string>& texId,
					std::optional<TextureSpecification>& outSpec)
					{
						if (!texId || texId->empty()) return;

						const std::filesystem::path texPath = AssetManager::Instance().ResolvePath(*texId);
						TextureSpecification spec = TextureConfigImporter::ImportTextureConfig(texPath.generic_string());
						outSpec = spec;
					};

				for (auto& [name, mesh] : modelPtr->GetMeshes())
				{
					Entity child = GetActiveScene().CreateEntity(name);
					child.AddComponent<MeshRendererComponent>();

					auto& mc = child.AddComponent<MeshComponent>(name, mesh, id);

					if (mesh->GetMaterial().has_value())
					{
						MaterialSpecification material = mesh->GetMaterial().value();

						fillTexSpec(material.AlbedoTexture, material.AlbedoTextureSpec);
						fillTexSpec(material.NormalTexture, material.NormalTextureSpec);
						fillTexSpec(material.MetallicTexture, material.MetallicTextureSpec);
						fillTexSpec(material.RoughnessTexture, material.RoughnessTextureSpec);
						fillTexSpec(material.AOTexture, material.AOTextureSpec);

						child.AddComponent<MaterialComponent>(material);
					}
					else
					{
						child.AddComponent<MaterialComponent>(MaterialSpecification{});
					}

					entity.GetComponent<HierarchyComponent>().AddChild(entity.GetUUID(), child.GetUUID());
				}
			}
		}
	}

	void SceneManager::LoadModel(const ModelToLoad& modelToLoad)
	{
		AssetToLoad asset;
		asset.id = std::filesystem::path(modelToLoad.path).filename().string();
		asset.path = modelToLoad.path;
		asset.type = MODEL;

		AssetManager::Instance().loadAsset(asset);

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

		for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
		{
			for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				vertices.insert(vertices.end(), { xPos, yPos, zPos, xPos, yPos, zPos, xSegment, ySegment });
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
		temp.AddComponent<MeshComponent>().GenerateMesh(vertices, indices, {}, DrawMode::TRIANGLE_STRIP);
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