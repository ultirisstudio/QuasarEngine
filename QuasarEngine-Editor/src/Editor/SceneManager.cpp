#include "SceneManager.h"

#include <iostream>
#include <filesystem>
#include <fstream>

#include <QuasarEngine.h>

#include "SceneSerializer.h"
#include "Tools/Utils.h"

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
						serializer.Deserialize(m_PendingActionFile);
						break;
					}
					case PendingAction::LoadSceneWithPath:
					{
						m_SceneObject->CreateScene();
						Renderer::BeginScene(m_SceneObject->GetScene());
						SceneSerializer serializer(*m_SceneObject, m_AssetPath);
						serializer.Deserialize(m_PendingActionFile);
						break;
					}
					case PendingAction::ReloadScene:
					{
						SceneSerializer serializer(*m_SceneObject, m_AssetPath);
						serializer.Deserialize(m_PendingActionFile);
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

				if (!Renderer::m_SceneData.m_AssetManager->isAssetLoaded(model.path))
				{
					m_ModelsToLoad.push(model);
					Renderer::m_SceneData.m_AssetManager->loadAsset({ model.path, MODEL });
					break;
				}

				std::shared_ptr<Model> modelPtr = Renderer::m_SceneData.m_AssetManager->getAsset<Model>(model.path);

				if (!modelPtr)
				{
					m_ModelsToLoad.push(model);
					break;
				}

				const size_t slash = model.path.find_last_of("/\\");
				const std::string m_SelectedFile = model.path.substr(slash + 1);

				size_t lastindex = m_SelectedFile.find_last_of(".");
				const std::string m_FileName = m_SelectedFile.substr(0, lastindex);

				const std::string m_FolderPath = model.path.substr(0, slash);

				std::filesystem::path path(m_FolderPath);

				Entity entity = GetActiveScene().CreateEntity(m_FileName);

				for (auto& [name, mesh] : modelPtr->GetMeshes())
				{
					Entity child = GetActiveScene().CreateEntity(name);
					child.AddComponent<MeshRendererComponent>();
					auto& mc = child.AddComponent<MeshComponent>(name, mesh, model.path);

					if (mesh->GetMaterial().has_value())
					{
						MaterialSpecification material = mesh->GetMaterial().value();
						if (material.AlbedoTexture.has_value())
						{
							//std::cout << "Albedo" << std::endl;
							std::filesystem::path finalPath(path.generic_string() + "/" + material.AlbedoTexture.value());
							//std::cout << finalPath << std::endl;

							/*if (!std::filesystem::exists(finalPath)) {
								std::cout << "Le fichier N'EXISTE PAS (filesystem) !" << std::endl;
							}
							else {
								std::cout << "Le fichier existe (filesystem) !" << std::endl;
							}*/

							std::ifstream file(finalPath);
							if (!file.is_open())
							{
								//std::cout << "ifstream NOT OPEN (direct path)" << std::endl;
								material.AlbedoTexture = std::nullopt;
							}
							else
							{
								//std::cout << "ifstream OK (direct path) !" << std::endl;
								material.AlbedoTexture = finalPath.generic_string();
							}
							file.close();
						}
						else
						{
							material.AlbedoTexture = std::nullopt;
						}

						if (material.NormalTexture.has_value())
						{
							std::filesystem::path finalPath(path.generic_string() + "/" + material.NormalTexture.value());
							std::ifstream file(finalPath.generic_string());
							if (!file.is_open())
							{
								material.NormalTexture = std::nullopt;
							}
							else
							{
								material.NormalTexture = finalPath.generic_string();
							}
							file.close();
						}

						if (material.MetallicTexture.has_value())
						{
							std::filesystem::path finalPath(path.generic_string() + "/" + material.MetallicTexture.value());
							std::ifstream file(finalPath.generic_string());
							if (!file.is_open())
							{
								material.MetallicTexture = std::nullopt;
							}
							else
							{
								material.MetallicTexture = finalPath.generic_string();
							}
							file.close();
						}

						if (material.RoughnessTexture.has_value())
						{
							std::filesystem::path finalPath(path.generic_string() + "/" + material.RoughnessTexture.value());
							std::ifstream file(finalPath.generic_string());
							if (!file.is_open())
							{
								material.RoughnessTexture = std::nullopt;
							}
							else
							{
								material.RoughnessTexture = finalPath.generic_string();
							}
							file.close();
						}

						if (material.AOTexture.has_value())
						{
							std::filesystem::path finalPath(path.generic_string() + "/" + material.AOTexture.value());
							std::ifstream file(finalPath.generic_string());
							if (!file.is_open())
							{
								material.AOTexture = std::nullopt;
							}
							else
							{
								material.AOTexture = finalPath.generic_string();
							}
							file.close();
						}

						child.AddComponent<MaterialComponent>(material);
					}
					else
					{
						MaterialSpecification material;
						//material.AlbedoTexture = "Assets/Textures/white_texture.jpg";

						child.AddComponent<MaterialComponent>(material);
					}

					entity.GetComponent<HierarchyComponent>().AddChild(entity.GetUUID(), child.GetUUID());
				}
			}
		}
	}

	void SceneManager::LoadModel(const ModelToLoad& modelToLoad)
	{
		AssetToLoad asset;
		asset.id = modelToLoad.path;
		asset.type = MODEL;

		Renderer::m_SceneData.m_AssetManager->loadAsset(asset);

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