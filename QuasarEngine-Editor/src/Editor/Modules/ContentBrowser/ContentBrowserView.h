#pragma once

#include <filesystem>

namespace QuasarEngine
{
    class ContentBrowserModel;
    class ContentBrowserActions;

    class ContentBrowserView
    {
    public:
        ContentBrowserView(ContentBrowserModel& model, ContentBrowserActions& actions);
        void Render();

    private:
        void DrawFolderTreePanel();
        void DrawFolderNode(const std::filesystem::path& dir);

        void DrawToolbar();
        void DrawBreadcrumbs();

        void DrawListView();
        void DrawGridView(int columnCount);
        void DrawPreviewPane(float rightPaneWidth);

        void DrawContextWindowPopup();
        void DrawDeleteConfirmModal();

        void HandleKeyboardShortcuts(bool windowFocused);

        void ItemContextMenu(const std::filesystem::path& absPath);

    private:
        ContentBrowserModel& m_Model;
        ContentBrowserActions& m_Actions;
    };
}
