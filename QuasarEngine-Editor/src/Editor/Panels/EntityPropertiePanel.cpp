#include "EntityPropertiePanel.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "glm/gtc/type_ptr.hpp"

#include <filesystem>
#include <sstream>

#include <QuasarEngine/Renderer/Renderer.h>
#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Entity/AllComponents.h>

#include <QuasarEngine/Core/UUID.h>

namespace QuasarEngine
{
	EntityPropertiePanel::EntityPropertiePanel()
	{
		m_TransformComponentPanel = std::make_unique<TransformComponentPanel>();
		m_CameraComponentPanel = std::make_unique<CameraComponentPanel>();
		m_MeshComponentPanel = std::make_unique<MeshComponentPanel>();
		m_TerrainComponentPanel = std::make_unique<TerrainComponentPanel>();
		m_MaterialComponentPanel = std::make_unique<MaterialComponentPanel>();
		m_LightComponentPanel = std::make_unique<LightComponentPanel>();
		m_MeshRendererComponentPanel = std::make_unique<MeshRendererComponentPanel>();
		m_RigidBodyComponentPanel = std::make_unique<RigidBodyComponentPanel>();
		m_BoxColliderComponentPanel = std::make_unique<BoxColliderComponentPanel>();
		m_MeshColliderComponentPanel = std::make_unique<MeshColliderComponentPanel>();
		m_CapsuleColliderComponentPanel = std::make_unique<CapsuleColliderComponentPanel>();
		m_SphereColliderComponentPanel = std::make_unique<SphereColliderComponentPanel>();
	}

	EntityPropertiePanel::~EntityPropertiePanel()
	{
		Renderer::m_SceneData.m_AssetManager->unloadAsset("Assets/Icons/no_texture.png");
	}

	void EntityPropertiePanel::OnImGuiRender(Scene& scene, SceneHierarchy& sceneHierarchy)
	{
		ImGui::Begin("Inspector");

		if (sceneHierarchy.m_SelectedEntity)
		{
			if (scene.isOnRuntime())
			{
				//ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				//ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}

			Entity entity = sceneHierarchy.m_SelectedEntity;

			UUID uuid = entity.GetUUID();

			std::string result;

			std::stringstream sstm;
			sstm << "UUID: " << uuid;
			result = sstm.str();
			ImGui::Text(result.c_str());
			ImGui::Separator();

			std::stringstream sstm2;
			sstm2 << "##" << uuid;
			result = sstm2.str();
			ImGui::InputText(result.c_str(), entity.GetName().data(), 20);

			ImGui::Separator();

			m_TransformComponentPanel->Render(entity);
			m_CameraComponentPanel->Render(entity, scene);
			m_MeshRendererComponentPanel->Render(entity);
			m_MeshComponentPanel->Render(entity);
			m_TerrainComponentPanel->Render(entity);
			m_MaterialComponentPanel->Render(entity);
			m_LightComponentPanel->Render(entity);
			m_RigidBodyComponentPanel->Render(entity);
			m_BoxColliderComponentPanel->Render(entity);
			m_MeshColliderComponentPanel->Render(entity);
			m_CapsuleColliderComponentPanel->Render(entity);
			m_SphereColliderComponentPanel->Render(entity);

			if (ImGui::Button("Add Component")) {
				ImGui::OpenPopup("AddComponent");
			}

			if (ImGui::BeginPopup("AddComponent"))
			{
				if (!entity.HasComponent<TransformComponent>()) {
					if (ImGui::MenuItem("Transform Component")) {
						entity.AddComponent<TransformComponent>();
					}
				}

				if (!entity.HasComponent<MeshRendererComponent>()) {
					if (ImGui::MenuItem("Mesh Renderer Component")) {
						entity.AddComponent<MeshRendererComponent>();
					}
				}

				if (!entity.HasComponent<MeshComponent>()) {
					if (ImGui::MenuItem("Mesh Component")) {
						if (!entity.HasComponent<MeshRendererComponent>()) {
							entity.AddComponent<MeshRendererComponent>();
						}
						entity.AddComponent<MeshComponent>();
					}
				}

				if (!entity.HasComponent<TerrainComponent>()) {
					if (ImGui::MenuItem("Terrain Component")) {
						entity.AddComponent<TerrainComponent>();
					}
				}

				if (!entity.HasComponent<MaterialComponent>()) {
					if (ImGui::MenuItem("Material Component")) {
						entity.AddComponent<MaterialComponent>();
						entity.GetComponent<MaterialComponent>();
					}
				}

				if (!entity.HasComponent<CameraComponent>()) {
					if (ImGui::MenuItem("Camera Component")) {
						entity.AddComponent<CameraComponent>().GetCamera().Init(&entity.GetComponent<TransformComponent>());
					}
				}

				if (!entity.HasComponent<LightComponent>()) {
					if (ImGui::MenuItem("Directional Light")) {
						entity.AddComponent<LightComponent>(LightComponent::LightType::DIRECTIONAL);
					}
				}

				if (!entity.HasComponent<LightComponent>()) {
					if (ImGui::MenuItem("Point Light Component")) {
						entity.AddComponent<LightComponent>(LightComponent::LightType::POINT);
					}
				}

				if (!entity.HasComponent<RigidBodyComponent>()) {
					if (ImGui::MenuItem("RigidBody Component")) {
						entity.AddComponent<RigidBodyComponent>().Init();
					}
				}

				if (!entity.HasComponent<BoxColliderComponent>()) {
					if (ImGui::MenuItem("Box Collider Component")) {
						entity.AddComponent<BoxColliderComponent>().Init();
					}
				}

				if (!entity.HasComponent<SphereColliderComponent>()) {
					if (ImGui::MenuItem("Sphere Collider Component")) {
						entity.AddComponent<SphereColliderComponent>().Init();
					}
				}

				if (!entity.HasComponent<MeshColliderComponent>()) {
					if (ImGui::MenuItem("Mesh Collider Component")) {
						entity.AddComponent<MeshColliderComponent>();
					}
				}

				if (!entity.HasComponent<CapsuleColliderComponent>()) {
					if (ImGui::MenuItem("Capsule Collider Component")) {
						entity.AddComponent<CapsuleColliderComponent>().Init();
					}
				}

				ImGui::EndPopup();
			}

			if (scene.isOnRuntime())
			{
				//ImGui::PopItemFlag();
				//ImGui::PopStyleVar();
			}
		}

		ImGui::End();
	}
}
