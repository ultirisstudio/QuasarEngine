#include "SceneHierarchy.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <QuasarEngine/Scene/SceneManager.h>

#include <QuasarEngine/Core/UUID.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Scene/Scene.h>

namespace QuasarEngine
{
	SceneHierarchy::SceneHierarchy(EditorContext& context) : IEditorModule(context)
	{
		
	}

	SceneHierarchy::~SceneHierarchy()
	{
		
	}

	void SceneHierarchy::Update(double dt)
	{

	}

	void SceneHierarchy::RenderUI()
	{
		ImGui::Begin("Scene Hierarchy");

		ImGuiID windowID = ImGui::GetCurrentWindow()->ID;

		Scene& scene = m_Context.sceneManager->GetActiveScene();

		if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->Rect(), windowID))
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_DRAG_AND_DROP_ENTITY"))
			{
				UUID droppedID = *(const UUID*)payload->Data;
				auto droppedEntity = scene.GetEntityByUUID(droppedID);

				if (droppedEntity)
				{
					UUID& droppedParent = droppedEntity->GetComponent<HierarchyComponent>().m_Parent;
					if (droppedParent != UUID::Null())
					{
						auto oldParent = scene.GetEntityByUUID(droppedParent);
						if (oldParent)
						{
							auto& siblings = oldParent->GetComponent<HierarchyComponent>().m_Childrens;
							siblings.erase(std::remove(siblings.begin(), siblings.end(), droppedID), siblings.end());
						}
						droppedParent = UUID::Null();
					}
				}
			}

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				// AddGameObject(path);
			}

			ImGui::EndDragDropTarget();
		}

		if (ImGui::BeginTable("SceneHierarchyTable", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame))
		{
			ImGui::TableSetupColumn("Nom", ImGuiTableColumnFlags_WidthFixed, 250.0f);
			ImGui::TableSetupColumn("UUID");
			ImGui::TableSetupColumn("Enfants");
			ImGui::TableSetupColumn("Actions");
			ImGui::TableHeadersRow();

			for (auto e : scene.GetAllEntitiesWith<IDComponent>())
			{
				Entity entity{ e, scene.GetRegistry() };
				UUID parentID = entity.HasComponent<HierarchyComponent>() ? entity.GetComponent<HierarchyComponent>().m_Parent : UUID::Null();
				if (parentID != UUID::Null())
					continue;

				OnDrawEntityNode(entity);
			}

			ImGui::EndTable();
		}

		ImGui::End();
	}

	void SceneHierarchy::OnDrawEntityNode(Entity entity)
	{
		Scene& scene = m_Context.sceneManager->GetActiveScene();

		ImGui::PushID((int)entity.GetUUID().value());
		bool deleteEntity = false;

		bool hasChildren = entity.HasComponent<HierarchyComponent>() ? !entity.GetComponent<HierarchyComponent>().m_Childrens.empty() : false;
		bool isSelected = m_Context.selectedEntity == entity;

		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;
		if (!hasChildren)
			treeFlags |= ImGuiTreeNodeFlags_Leaf;
		if (isSelected)
			treeFlags |= ImGuiTreeNodeFlags_Selected;
		if (hasChildren)
			treeFlags |= ImGuiTreeNodeFlags_OpenOnArrow;

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		bool open = ImGui::TreeNodeEx((void*)(intptr_t)entity.GetUUID().value(), treeFlags, "%s", entity.GetName().c_str());

		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Supprimer")) {
				if (m_Context.selectedEntity == entity)
					m_Context.selectedEntity = {};
				scene.DestroyEntity(entity.GetUUID());
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropSource()) {
			UUID id = entity.GetUUID();
			ImGui::Text("%s", entity.GetName().c_str());
			ImGui::SetDragDropPayload("SCENE_DRAG_AND_DROP_ENTITY", &id, sizeof(UUID));

			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_DRAG_AND_DROP_ENTITY"))
			{
				UUID droppedID = *(const UUID*)payload->Data;

				auto droppedEntity = scene.GetEntityByUUID(droppedID);
				if (droppedEntity && droppedEntity.value() != entity)
				{
					UUID& droppedParent = droppedEntity->GetComponent<HierarchyComponent>().m_Parent;
					if (droppedParent != UUID::Null()) {
						auto oldParent = scene.GetEntityByUUID(droppedParent);
						if (oldParent) {
							auto& siblings = oldParent->GetComponent<HierarchyComponent>().m_Childrens;
							siblings.erase(std::remove(siblings.begin(), siblings.end(), droppedID), siblings.end());
						}
					}
					droppedParent = entity.GetUUID();
					entity.GetComponent<HierarchyComponent>().m_Childrens.push_back(droppedID);
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemClicked())
			m_Context.selectedEntity = entity;

		ImGui::TableSetColumnIndex(1);
		std::string uuidStr = entity.GetUUID().ToString();
		if (uuidStr.length() > 6)
			uuidStr = uuidStr.substr(0, 6) + "...";
		ImGui::TextUnformatted(uuidStr.c_str());

		ImGui::TableSetColumnIndex(2);
		size_t childrenCount = entity.HasComponent<HierarchyComponent>() ? entity.GetComponent<HierarchyComponent>().m_Childrens.size() : 0;
		ImGui::Text("%zu", childrenCount);

		ImGui::TableSetColumnIndex(3); // 4
		float buttonSize = 20.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

		if (ImGui::Button("+", ImVec2(buttonSize, buttonSize))) {
			Entity child = scene.CreateEntity("Nouveau");
			entity.GetComponent<HierarchyComponent>().m_Childrens.push_back(child.GetUUID());
			child.GetComponent<HierarchyComponent>().m_Parent = entity.GetUUID();
		}
		ImGui::SameLine();

		if (ImGui::Button("X", ImVec2(buttonSize, buttonSize))) {
			if (m_Context.selectedEntity == entity)
				m_Context.selectedEntity = {};
			deleteEntity = true;
		}
		ImGui::PopStyleVar();

		if (open) {
			for (UUID childID : entity.GetComponent<HierarchyComponent>().m_Childrens) {
				std::optional<Entity> child = scene.GetEntityByUUID(childID);
				if (child.has_value())
					OnDrawEntityNode(child.value());
			}
			ImGui::TreePop();
		}

		ImGui::PopID();

		if (deleteEntity) {
			scene.DestroyEntity(entity.GetUUID());
		}
	}
}