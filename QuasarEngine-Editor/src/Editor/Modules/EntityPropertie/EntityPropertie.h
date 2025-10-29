#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace QuasarEngine
{
    class Scene;
    class SceneHierarchy;
    class Entity;
    class IComponentPanel;

    class EntityPropertie
    {
    public:
        explicit EntityPropertie(const std::string& projectPath);
        ~EntityPropertie();

        void OnImGuiRender(SceneHierarchy& sceneHierarchy);

    private:
        struct PanelEntry
        {
            std::unique_ptr<IComponentPanel> panel;
            std::string name;
        };

        struct MenuItem
        {
            std::string name;
            std::function<bool(Entity&)> hasComponent;
            std::function<void(Entity&)> addComponent;
            std::string category;
            std::string keywords;
        };

        void buildPanels(const std::string& projectPath);
        void buildMenuItems(const std::string& projectPath);
        void renderPanels(Entity entity);
        void renderAddComponentPopup(Entity entity);

        static bool textContainsI(const std::string& hay, const std::string& needle);

        std::vector<PanelEntry> m_Panels;
        std::vector<MenuItem> m_MenuItems;

        std::string m_ProjectPath;

        char m_SearchBuffer[128] = { 0 };
    };
}