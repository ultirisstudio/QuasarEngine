#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <Editor/Modules/IEditorModule.h>

#include <QuasarEngine/Entity/Entity.h>

namespace QuasarEngine
{
    class IComponentPanel;

    class EntityPropertie : public IEditorModule
    {
    public:
        EntityPropertie(EditorContext& context);
        ~EntityPropertie() override;

        void Update(double dt) override;
		void Render() override;
        void RenderUI() override;

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

        std::vector<PanelEntry> m_Panels;
        std::vector<MenuItem> m_MenuItems;

        char m_SearchBuffer[128] = { 0 };
    };
}