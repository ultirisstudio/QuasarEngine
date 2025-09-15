#pragma once

#include <filesystem>
#include <unordered_map>
#include <bitset>

#include <QuasarEngine.h>
#include <QuasarEngine/Events/Event.h>
#include <QuasarEngine/Events/KeyEvent.h>
#include <QuasarEngine/Events/MouseEvent.h>
#include <QuasarEngine/Events/ApplicationEvent.h>
#include <QuasarEngine/Resources/Texture.h>
#include <QuasarEngine/Tools/Chronometer.h>

#include <QuasarEngine/Shader/Shader.h>
#include <QuasarEngine/Resources/Model.h>
#include <QuasarEngine/Resources/Texture2D.h>
#include <QuasarEngine/Resources/Materials/Material.h>

#include "Panels/ContentBrowserPanel.h"
#include "Panels/EntityPropertiePanel.h"
#include "Panels/SceneHierarchy.h"
#include "Panels/EditorViewport.h"
#include "Panels/Viewport.h"
#include "Panels/NodeEditor/NodeEditor.h"
#include "Panels/AnimationEditorPanel.h"
#include "Panels/HeightMapEditor/HeightMapEditor.h"
#include "Panels/UI/UserInterfaceEditor.h"

#include "QuasarEngine/Scene/Scene.h"

#include "Importer/AssetImporter.h"

#include "SceneManager.h"

namespace QuasarEngine
{
	constexpr int FRAME_HISTORY_SIZE = 120;

	struct FrameTimeHistory
	{
		float begin[FRAME_HISTORY_SIZE] = {};
		float update[FRAME_HISTORY_SIZE] = {};
		float render[FRAME_HISTORY_SIZE] = {};
		float end[FRAME_HISTORY_SIZE] = {};
		float event[FRAME_HISTORY_SIZE] = {};
		float asset[FRAME_HISTORY_SIZE] = {};
		float imgui[FRAME_HISTORY_SIZE] = {};
		int index = 0;
		bool filled = false;

		void Push(const ApplicationInfos& infos) {
			begin[index] = (float)infos.begin_latency;
			update[index] = (float)infos.update_latency;
			render[index] = (float)infos.render_latency;
			end[index] = (float)infos.end_latency;
			event[index] = (float)infos.event_latency;
			asset[index] = (float)infos.asset_latency;
			imgui[index] = (float)infos.imgui_latency;
			index = (index + 1) % FRAME_HISTORY_SIZE;
			if (index == 0) filled = true;
		}
		int Size() const { return filled ? FRAME_HISTORY_SIZE : index; }
	};

	struct EditorSpecification
	{
		std::string EngineExecutablePath;

		std::string ProjectName;
		std::string ProjectPath;
	};

	enum MenuType : uint32_t
	{
		OPTION_MENU = 0
	};

	class Editor : public Layer
	{
	public:
		Editor(const EditorSpecification& spec);
		virtual ~Editor() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(double dt) override;
		void OnRender() override;
		void OnGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);
	private:
		void InitImGuiStyle();
		void OptionMenu();

		void DrawStatsWindow();
		void DrawFrameStats(const ApplicationInfos& infos, FrameTimeHistory& history);

		void SetupAssets(const std::filesystem::path& chemin);

		EditorSpecification m_Specification;

		std::unique_ptr<ContentBrowserPanel> m_ContentBrowserPanel;
		std::unique_ptr<EntityPropertiePanel> m_EntityPropertiePanel;
		std::unique_ptr<SceneHierarchy> m_SceneHierarchy;
		std::unique_ptr<EditorViewport> m_EditorViewport;
		std::unique_ptr<Viewport> m_Viewport;
		//std::unique_ptr<NodeEditor> m_NodeEditor;
		//std::unique_ptr<AnimationEditorPanel> m_AnimationEditorPanel;
		//std::unique_ptr<HeightMapEditor> m_HeightMapEditor;
		//std::unique_ptr<UserInterfaceEditor> m_UserInterfaceEditor;

		std::unique_ptr<SceneManager> m_SceneManager;
		std::unique_ptr<EditorCamera> m_EditorCamera;

		std::unique_ptr<AssetImporter> m_AssetImporter;

		bool m_optionMenu = false;
		int m_optionTab = 0;

		std::bitset<1> menu_state;

		std::vector<unsigned int> m_ImGuiColor;
		std::vector<const char*> m_ThemeName;
		std::map<unsigned int, glm::vec4> m_ThemeColor;
	};
}