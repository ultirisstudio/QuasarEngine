#include "SceneHierarchy.h"

#include <imgui/imgui.h>

#include <QuasarEngine/Core/UUID.h>
#include <QuasarEngine/Entity/Components/HierarchyComponent.h>

namespace QuasarEngine
{
	SceneHierarchy::SceneHierarchy()
	{
		m_SelectedEntity = {};
	}

	void SceneHierarchy::OnImGuiRender(Scene& scene)
	{
		ImGui::Begin("Scene Hierarchy");

		if (ImGui::BeginTable("SceneHierarchyTable", 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame))
		{
			ImGui::TableSetupColumn("Nom", ImGuiTableColumnFlags_WidthFixed, 250.0f);
			ImGui::TableSetupColumn("UUID");
			ImGui::TableSetupColumn("Enfants");
			ImGui::TableSetupColumn("Type");
			ImGui::TableSetupColumn("Actions");
			ImGui::TableHeadersRow();

			for (auto e : scene.GetAllEntitiesWith<IDComponent>())
			{
				Entity entity{ e, scene.GetRegistry() };
				if (entity.GetComponent<HierarchyComponent>().m_Parent != UUID::Null())
					continue;

				OnDrawEntityNode(scene, entity);
			}

			ImGui::EndTable();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* path = (const wchar_t*)payload->Data;
				// AddGameObject(path);
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::End();
	}

	void SceneHierarchy::OnDrawEntityNode(Scene& scene, Entity entity)
	{
		ImGui::PushID((int)entity.GetUUID());
		bool deleteEntity = false;

		bool hasChildren = !entity.GetComponent<HierarchyComponent>().m_Childrens.empty();
		bool isSelected = m_SelectedEntity == entity;

		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;
		if (!hasChildren)
			treeFlags |= ImGuiTreeNodeFlags_Leaf;
		if (isSelected)
			treeFlags |= ImGuiTreeNodeFlags_Selected;
		if (hasChildren)
			treeFlags |= ImGuiTreeNodeFlags_OpenOnArrow;

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);

		bool open = ImGui::TreeNodeEx((void*)(intptr_t)entity.GetUUID(), treeFlags, "%s", entity.GetName().c_str());
		if (ImGui::IsItemClicked())
			m_SelectedEntity = entity;

		ImGui::TableSetColumnIndex(1);
		std::string uuidStr = std::to_string(entity.GetUUID());
		if (uuidStr.length() > 6)
			uuidStr = uuidStr.substr(0, 6) + "...";
		ImGui::TextUnformatted(uuidStr.c_str());

		ImGui::TableSetColumnIndex(2);
		size_t childrenCount = entity.GetComponent<HierarchyComponent>().m_Childrens.size();
		ImGui::Text("%zu", childrenCount);

		ImGui::TableSetColumnIndex(3);
		if (entity.HasComponent<TagComponent>())
			ImGui::TextUnformatted(entity.GetComponent<TagComponent>().Tag.c_str());
		else
			ImGui::TextUnformatted("-");

		ImGui::TableSetColumnIndex(4);
		float buttonSize = 20.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

		if (ImGui::Button("V", ImVec2(buttonSize, buttonSize))) {
			
		}
		ImGui::SameLine();

		if (ImGui::Button("+", ImVec2(buttonSize, buttonSize))) {
			Entity child = scene.CreateEntity("Nouveau");
			entity.GetComponent<HierarchyComponent>().m_Childrens.push_back(child.GetUUID());
			child.GetComponent<HierarchyComponent>().m_Parent = entity.GetUUID();
		}
		ImGui::SameLine();

		if (ImGui::Button("X", ImVec2(buttonSize, buttonSize))) {
			if (m_SelectedEntity == entity)
				m_SelectedEntity = {};
			deleteEntity = true;
		}
		ImGui::PopStyleVar();

		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Supprimer")) {
				if (m_SelectedEntity == entity)
					m_SelectedEntity = {};
				scene.DestroyEntity(entity);
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginDragDropSource()) {
			std::wstring wideStr = std::wstring(std::to_string(entity.GetUUID()).begin(), std::to_string(entity.GetUUID()).end());
			const wchar_t* uuid = wideStr.c_str();
			ImGui::Text("%s", entity.GetName().c_str());
			ImGui::SetDragDropPayload("SCENE_DRAG_AND_DROP_ENTITY", uuid, (wcslen(uuid) + 1) * sizeof(wchar_t));
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_DRAG_AND_DROP_ENTITY")) {
				const wchar_t* data = (const wchar_t*)payload->Data;
				UUID droppedID = std::stoull(std::string(data, data + wcslen(data)));
				std::optional<Entity> droppedEntity = scene.GetEntityByUUID(droppedID);

				if (droppedEntity.has_value())
				{
					if (droppedEntity.value() != entity)
					{
						UUID& droppedParent = droppedEntity.value().GetComponent<HierarchyComponent>().m_Parent;
						if (droppedParent != UUID::Null()) {
							std::optional<Entity> oldParent = scene.GetEntityByUUID(droppedParent);
							if (oldParent.has_value())
							{
								auto& siblings = oldParent.value().GetComponent<HierarchyComponent>().m_Childrens;
								siblings.erase(std::remove(siblings.begin(), siblings.end(), droppedEntity.value().GetUUID()), siblings.end());
							}
						}
						droppedParent = entity.GetUUID();
						entity.GetComponent<HierarchyComponent>().m_Childrens.push_back(droppedEntity.value().GetUUID());
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (open) {
			for (UUID childID : entity.GetComponent<HierarchyComponent>().m_Childrens) {
				std::optional<Entity> child = scene.GetEntityByUUID(childID);
				if (child.has_value())
					OnDrawEntityNode(scene, child.value());
			}
			ImGui::TreePop();
		}

		ImGui::PopID();

		if (deleteEntity) {
			scene.DestroyEntity(entity);
		}
	}
}