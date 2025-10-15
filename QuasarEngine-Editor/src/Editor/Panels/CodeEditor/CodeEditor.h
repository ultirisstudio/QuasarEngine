#pragma once

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <imgui/imgui.h>
#include <TextEditor.h>

namespace QuasarEngine
{
    struct CodeEditorDiagnostics {
        int line;
        std::string message;
        TextEditor::PaletteIndex severity;
    };

    class CodeEditor
    {
    public:
        CodeEditor();

        void OnImGuiRender(const char* id = "CodeEditor");

        bool LoadFromFile(const std::string& path);
        bool SaveToFile(const std::string& path);
        void SetText(const std::string& text);
        std::string GetText() const;

        void SetLanguageDefinition(const TextEditor::LanguageDefinition& lang);
        void SetPalette(const TextEditor::Palette& palette);

        void SetFont(ImFont* font, float size = 0.f);

        void SetAutoCompleteDictionary(const std::vector<std::string>& dict);
        void AddToAutoCompleteDictionary(const std::string& word);

        void SetDiagnostics(const std::vector<CodeEditorDiagnostics>& diags);

        bool IsTextChanged() const;
        void Undo();
        void Redo();

        std::function<void()> OnTextChanged;
        std::function<void(const CodeEditorDiagnostics&)> OnDiagnosticClicked;

    private:
        void RenderSearchBar();
        void RenderContextMenu();
        void RenderAutoCompletePopup(const ImVec2& position, const std::string& currentWord);

        void UpdateAutoCompleteSuggestions(const std::string& currentWord);

        std::string GetCurrentWord() const;
        void InsertAutoComplete(const std::string& word);

    private:
        TextEditor editor;
        ImFont* font = nullptr;
        float font_size = 0.f;

        std::string currentFile;
        bool textChanged = false;

        std::vector<std::string> dictionary;
        std::vector<std::string> suggestions;
        int selectedSuggestion = 0;
        bool showAutoComplete = false;

        char searchBuffer[128] = "";
        char replaceBuffer[128] = "";
        bool searching = false;
        bool replaceMode = false;

        std::vector<CodeEditorDiagnostics> diagnostics;
    };
}