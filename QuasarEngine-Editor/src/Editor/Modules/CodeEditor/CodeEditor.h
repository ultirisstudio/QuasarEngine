#pragma once

#include <string>
#include <vector>
#include <functional>
#include <optional>

#include <imgui/imgui.h>

#include <TextEditor.h>

#include <Editor/Modules/IEditorModule.h>

namespace QuasarEngine
{
    struct CodeEditorDiagnostics {
        int line;
        std::string message;
        TextEditor::PaletteIndex severity;
    };

    class CodeEditor : public IEditorModule
    {
    public:
        CodeEditor(EditorContext& context);
        ~CodeEditor() override;

        void Update(double dt) override;
        void Render() override;
        void RenderUI() override;

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

        enum class SuggestionKind { Keyword, Identifier, Function, Snippet, FilePath, Unknown };

        struct Suggestion {
            std::string label;
            std::string insertText;
            SuggestionKind kind = SuggestionKind::Unknown;
            std::string detail;
            std::string doc;
            int score = 0;
        };

        using SuggestionProvider = std::function<void(const std::string& word, std::vector<Suggestion>& out)>;

        void AddSuggestionProvider(const SuggestionProvider& provider);
        void SetKeywords(const std::vector<std::string>& kw);
        void SetSnippets(const std::vector<Suggestion>& snippets);

        void SetZoom(float factor);
        float GetZoom() const { return m_Zoom; }
        void ZoomIn();
        void ZoomOut();
        void ResetZoom();

        std::function<void()> OnTextChanged;
        std::function<void(const CodeEditorDiagnostics&)> OnDiagnosticClicked;

    private:
        void RenderSearchBar();
        void RenderContextMenu();
        void RenderAutoCompletePopup(const ImVec2& position, const std::string& currentWord);

        void UpdateAutoCompleteSuggestions(const std::string& currentWord);

        std::string GetCurrentWord() const;
        void InsertAutoComplete(const std::string& word);

        std::pair<TextEditor::Coordinates, TextEditor::Coordinates> GetCurrentWordRange() const;

        static int FuzzyScore(const std::string& query, const std::string& target);

        void ProviderDictionary(const std::string& w, std::vector<Suggestion>& out);
        void ProviderBufferIdentifiers(const std::string& w, std::vector<Suggestion>& out);
        void ProviderKeywords(const std::string& w, std::vector<Suggestion>& out);
        void ProviderSnippets(const std::string& w, std::vector<Suggestion>& out);
        void RebuildBufferIdentifiers();

        std::string SliceTextFromRange(const TextEditor::Coordinates& coord_from, const TextEditor::Coordinates& coord_to) const;

    private:
        TextEditor editor;
        ImFont* font = nullptr;
        float font_size = 0.f;

        std::string currentFile;
        bool textChanged = false;

        std::vector<std::string> dictionary;
        std::vector<std::string> bufferIdentifiers;
        std::vector<std::string> keywords;

        std::vector<Suggestion> suggestions;
        std::vector<Suggestion> snippetBank;

        std::vector<SuggestionProvider> providers;

        int selectedSuggestion = 0;
        bool showAutoComplete = false;

        char searchBuffer[128] = "";
        char replaceBuffer[128] = "";
        bool searching = false;
        bool replaceMode = false;

        float m_Zoom = 1.0f;
        float m_MinZoom = 0.5f;
        float m_MaxZoom = 3.0f;
        float m_ZoomStep = 0.1f;

        int m_SelectedSuggestion = 0;
        bool m_ShowAutoComplete = false;
        int m_MaxSuggestions = 100;

        std::vector<CodeEditorDiagnostics> diagnostics;
    };
}