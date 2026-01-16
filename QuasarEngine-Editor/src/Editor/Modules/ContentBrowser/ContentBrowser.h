#pragma once

#include <memory>

#include <Editor/Modules/IEditorModule.h>

namespace QuasarEngine
{
    struct EditorContext;

    class ContentBrowserModel;
    class ContentBrowserActions;
    class ContentBrowserView;

    class ContentBrowser : public IEditorModule
    {
    public:
        ContentBrowser(EditorContext& context);
        ~ContentBrowser() override;

        void Update(double dt) override;
        void RenderUI() override;

    private:
        std::unique_ptr<ContentBrowserModel> m_Model;
        std::unique_ptr<ContentBrowserActions> m_Actions;
        std::unique_ptr<ContentBrowserView> m_View;
    };
}
