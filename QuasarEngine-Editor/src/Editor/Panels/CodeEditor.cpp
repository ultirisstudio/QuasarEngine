#include "CodeEditor.h"
#include <imgui/imgui_internal.h>
#include <fstream>
#include <algorithm>
#include <cstring>

namespace QuasarEngine
{
    CodeEditor::CodeEditor()
    {
        editor.SetShowWhitespaces(false);
        editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
        editor.SetPalette(editor.GetDarkPalette());
        editor.SetTabSize(4);
        font = nullptr;
        font_size = 0.f;
    }

    void CodeEditor::SetFont(ImFont* _font, float size)
    {
        font = _font;
        font_size = size;
    }

    void CodeEditor::SetLanguageDefinition(const TextEditor::LanguageDefinition& lang)
    {
        editor.SetLanguageDefinition(lang);
    }
    void CodeEditor::SetPalette(const TextEditor::Palette& palette)
    {
        editor.SetPalette(palette);
    }

    void CodeEditor::SetText(const std::string& text)
    {
        editor.SetText(text);
        textChanged = false;
    }

    std::string CodeEditor::GetText() const
    {
        return editor.GetText();
    }

    bool CodeEditor::LoadFromFile(const std::string& path)
    {
        std::ifstream in(path);
        if (!in.is_open()) return false;
        std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        SetText(text);
        currentFile = path;
        textChanged = false;
        return true;
    }

    bool CodeEditor::SaveToFile(const std::string& path)
    {
        std::ofstream out(path);
        if (!out.is_open()) return false;
        out << editor.GetText();
        currentFile = path;
        textChanged = false;
        return true;
    }

    void CodeEditor::SetAutoCompleteDictionary(const std::vector<std::string>& dict)
    {
        dictionary = dict;
    }
    void CodeEditor::AddToAutoCompleteDictionary(const std::string& word)
    {
        dictionary.push_back(word);
    }
    void CodeEditor::SetDiagnostics(const std::vector<CodeEditorDiagnostics>& diags)
    {
        diagnostics = diags;
        TextEditor::ErrorMarkers markers;
        for (const auto& diag : diags)
            markers[diag.line] = diag.message;
        editor.SetErrorMarkers(markers);
    }

    bool CodeEditor::IsTextChanged() const
    {
        return editor.IsTextChanged() || textChanged;
    }

    void CodeEditor::Undo() { editor.Undo(); }
    void CodeEditor::Redo() { editor.Redo(); }

    std::string CodeEditor::GetCurrentWord() const
    {
        auto pos = editor.GetCursorPosition();
        std::string lineText = editor.GetCurrentLineText();
        if (pos.mLine < editor.GetTotalLines() && pos.mColumn > 0 && !lineText.empty()) {
            int col = pos.mColumn - 1;
            std::string currentWord;
            while (col >= 0 && (isalnum(static_cast<unsigned char>(lineText[col])) || lineText[col] == '_')) {
                currentWord = lineText[col] + currentWord;
                --col;
            }
            return currentWord;
        }
        return {};
    }

    void CodeEditor::UpdateAutoCompleteSuggestions(const std::string& currentWord)
    {
        suggestions.clear();
        if (currentWord.empty()) return;
        for (const auto& word : dictionary) {
            if (word.rfind(currentWord, 0) == 0)
                suggestions.push_back(word);
        }
        selectedSuggestion = 0;
        showAutoComplete = !suggestions.empty();
    }

    void CodeEditor::InsertAutoComplete(const std::string& word)
    {
        auto pos = editor.GetCursorPosition();
        std::string currentWord = GetCurrentWord();
        TextEditor::Coordinates from = { pos.mLine, pos.mColumn - (int)currentWord.length() };
        TextEditor::Coordinates to = pos;
        editor.SetSelection(from, to);
        editor.DeleteSelection();
        editor.InsertText(word);
    }

    void CodeEditor::RenderContextMenu()
    {
        if (ImGui::BeginPopupContextWindow("CodeEditorContextMenu")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) Undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) Redo();
            ImGui::Separator();
            if (ImGui::MenuItem("Copy", "Ctrl+C")) editor.Copy();
            if (ImGui::MenuItem("Paste", "Ctrl+V")) editor.Paste();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) editor.Cut();
            ImGui::Separator();
            if (ImGui::MenuItem("Duplicate Line", "Ctrl+D")) {
                auto pos = editor.GetCursorPosition();
                int lineIdx = pos.mLine;
                const auto& lines = editor.GetTextLines();
                if (lineIdx >= 0 && lineIdx < lines.size())
                {
                    std::string line = lines[lineIdx];
                    editor.InsertText(lines[pos.mLine] + "\n");
                }
            }
            if (ImGui::MenuItem("Select All", "Ctrl+A")) editor.SelectAll();
            ImGui::EndPopup();
        }
    }

    void CodeEditor::RenderSearchBar()
    {
        ImGui::BeginGroup();
        ImGui::PushItemWidth(120);
        ImGui::InputTextWithHint("##search", "Search...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Find")) {
            searching = true;
            // Go to next match
            std::string text = editor.GetText();
            std::string search(searchBuffer);
            auto pos = text.find(search);
            if (pos != std::string::npos) {
                int line = 0, col = 0, c = 0;
                for (char ch : text) {
                    if (c == pos) break;
                    if (ch == '\n') { line++; col = 0; }
                    else col++;
                    c++;
                }
                editor.SetCursorPosition({ line, col });
                editor.SetSelection({ line, col }, { line, col + (int)search.length() });
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Replace ?", &replaceMode);
        if (replaceMode) {
            ImGui::SameLine();
            ImGui::PushItemWidth(120);
            ImGui::InputTextWithHint("##replace", "Replace...", replaceBuffer, IM_ARRAYSIZE(replaceBuffer));
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Replace")) {
                std::string text = editor.GetText();
                std::string search(searchBuffer);
                std::string replaceStr(replaceBuffer);
                if (!search.empty()) {
                    size_t pos = 0;
                    while ((pos = text.find(search, pos)) != std::string::npos) {
                        text.replace(pos, search.length(), replaceStr);
                        pos += replaceStr.length();
                    }
                    SetText(text);
                }
            }
        }
        ImGui::EndGroup();
    }

    void CodeEditor::RenderAutoCompletePopup(const ImVec2& pos, const std::string& currentWord)
    {
        ImGui::SetNextWindowBgAlpha(0.92f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(220, std::min(200.f, 18.f + suggestions.size() * 18.f)), ImGuiCond_Always);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        ImGui::Begin("AutoCompletePopup", nullptr, flags);
        for (size_t i = 0; i < suggestions.size(); ++i) {
            bool isSelected = (i == selectedSuggestion);
            if (ImGui::Selectable(suggestions[i].c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups)) {
                InsertAutoComplete(suggestions[i]);
                showAutoComplete = false;
                ImGui::End();
                return;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            selectedSuggestion = (selectedSuggestion + 1) % (int)suggestions.size();
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            selectedSuggestion = (selectedSuggestion - 1 + (int)suggestions.size()) % (int)suggestions.size();
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Tab)) {
            InsertAutoComplete(suggestions[selectedSuggestion]);
            showAutoComplete = false;
            ImGui::End();
            return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            showAutoComplete = false;
        ImGui::End();
    }

    void CodeEditor::OnImGuiRender(const char* id)
    {
        std::string windowTitle = "Code Editor";
        if (id && *id)
            windowTitle += "##" + std::string(id);

        if (ImGui::Begin(windowTitle.c_str()))

        if (font)
            ImGui::PushFont(font);

        if (!currentFile.empty()) {
            ImGui::Text("%s", currentFile.c_str());
            ImGui::SameLine();
        }
        if (IsTextChanged()) {
            ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "*");
            ImGui::SameLine();
        }
        if (ImGui::Button("Save (Ctrl+S)")) {
            if (!currentFile.empty()) SaveToFile(currentFile);
        }
        ImGui::SameLine();
        //ImGui::Checkbox("Line Numbers", &editor.mShowLineNumbers);

        if (ImGui::Button("Find (Ctrl+F)")) searching = !searching;
        if (searching) RenderSearchBar();

        for (const auto& diag : diagnostics) {
            ImGui::PushStyleColor(ImGuiCol_Text, diag.severity == TextEditor::PaletteIndex::ErrorMarker ? ImVec4(1, 0.3f, 0.3f, 1) : ImVec4(1, 1, 0, 1));
            if (ImGui::Selectable((std::to_string(diag.line + 1) + ": " + diag.message).c_str())) {
                if (OnDiagnosticClicked) OnDiagnosticClicked(diag);
                editor.SetCursorPosition({ diag.line, 0 });
            }
            ImGui::PopStyleColor();
        }

        editor.Render(id);

        RenderContextMenu();

        std::string currentWord = GetCurrentWord();
        if (!currentWord.empty() && (ImGui::IsKeyPressed(ImGuiKey_Space) && ImGui::GetIO().KeyCtrl)) {
            UpdateAutoCompleteSuggestions(currentWord);
        }
        if (!showAutoComplete && !currentWord.empty()) {
            UpdateAutoCompleteSuggestions(currentWord);
        }
        if (showAutoComplete && !suggestions.empty()) {
            ImVec2 popupPos = ImGui::GetCursorScreenPos() + ImVec2(currentWord.length() * 8.f, 24.f);
            RenderAutoCompletePopup(popupPos, currentWord);
        }

        if (ImGui::IsKeyPressed(ImGuiKey_S, false) && ImGui::GetIO().KeyCtrl) {
            if (!currentFile.empty()) SaveToFile(currentFile);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F, false) && ImGui::GetIO().KeyCtrl) {
            searching = true;
        }

        if (font)
            ImGui::PopFont();

        if (editor.IsTextChanged()) {
            textChanged = true;
            if (OnTextChanged) OnTextChanged();
        }

        ImGui::End();
    }
}