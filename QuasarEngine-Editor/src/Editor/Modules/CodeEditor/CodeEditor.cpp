#include "CodeEditor.h"
#include <imgui/imgui_internal.h>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <set>

namespace QuasarEngine
{
    CodeEditor::CodeEditor()
    {
        editor.SetShowWhitespaces(false);
        editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        editor.SetPalette(editor.GetDarkPalette());
        editor.SetTabSize(4);
        font = nullptr;
        font_size = 0.f;

        providers.push_back([this](const std::string& w, std::vector<Suggestion>& out) { ProviderKeywords(w, out); });
        providers.push_back([this](const std::string& w, std::vector<Suggestion>& out) { ProviderBufferIdentifiers(w, out); });
        providers.push_back([this](const std::string& w, std::vector<Suggestion>& out) { ProviderDictionary(w, out); });
        providers.push_back([this](const std::string& w, std::vector<Suggestion>& out) { ProviderSnippets(w, out); });
    }

    void CodeEditor::SetFont(ImFont* _font, float size)
    {
        font = _font;
        font_size = size;
        if (font && size > 0.f) {
            SetZoom(size / font->FontSize);
        }
    }


    void CodeEditor::SetLanguageDefinition(const TextEditor::LanguageDefinition& lang)
    {
        editor.SetLanguageDefinition(lang);

        if (lang.mName == "Lua")
        {
            SetKeywords({ "and","break","do","else","elseif","end","false","for","function","if","in","local","nil","not","or","repeat","return","then","true","until","while" });

            SetSnippets({
                {"func", "function ${1:name}(${2:args})\n\t$0\nend", SuggestionKind::Snippet, "snippet", "Déclare une fonction Lua"},
                {"fori", "for ${1:i}=1,${2:n} do\n\t$0\nend", SuggestionKind::Snippet, "snippet", "Boucle for i=1..n"}
                });
        }

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

    void CodeEditor::AddSuggestionProvider(const SuggestionProvider& p) { providers.push_back(p); }

    void CodeEditor::SetKeywords(const std::vector<std::string>& kw) { keywords = kw; }

    void CodeEditor::SetSnippets(const std::vector<Suggestion>& sn) { snippetBank = sn; }

    bool CodeEditor::IsTextChanged() const
    {
        return editor.IsTextChanged() || textChanged;
    }

    void CodeEditor::Undo() { editor.Undo(); }
    void CodeEditor::Redo() { editor.Redo(); }

    void CodeEditor::SetZoom(float factor)
    {
        m_Zoom = std::clamp(factor, m_MinZoom, m_MaxZoom);
    }

    void CodeEditor::ZoomIn() { SetZoom(m_Zoom + m_ZoomStep); }
    void CodeEditor::ZoomOut() { SetZoom(m_Zoom - m_ZoomStep); }
    void CodeEditor::ResetZoom() { SetZoom(1.0f); }

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

    /*std::string CodeEditor::GetCurrentWord() const
    {
        auto range = GetCurrentWordRange();
        if (range.first == range.second) return {};
        return SliceTextFromRange(range.first, range.second);
    }*/

    void CodeEditor::UpdateAutoCompleteSuggestions(const std::string& currentWord)
    {
        suggestions.clear();
        if (currentWord.empty()) { showAutoComplete = false; return; }

        for (auto& p : providers) p(currentWord, suggestions);

        std::unordered_map<std::string, int> best;
        std::vector<Suggestion> uniq;
        uniq.reserve(suggestions.size());
        for (auto& s : suggestions) {
            auto it = best.find(s.label);
            if (it == best.end() || s.score > it->second) {
                best[s.label] = s.score;
            }
        }
        for (auto& [lab, sc] : best) {
            for (auto& s : suggestions) {
                if (s.label == lab && s.score == sc) { uniq.push_back(s); break; }
            }
        }
        suggestions.swap(uniq);

        auto kindBias = [](SuggestionKind k) {
            switch (k) {
            case SuggestionKind::Keyword: return 3;
            case SuggestionKind::Identifier: return 2;
            case SuggestionKind::Function: return 2;
            case SuggestionKind::Snippet: return 1;
            default: return 0;
            }
            };
        std::sort(suggestions.begin(), suggestions.end(), [&](const Suggestion& a, const Suggestion& b) {
            if (a.score != b.score) return a.score > b.score;
            int ka = kindBias(a.kind), kb = kindBias(b.kind);
            if (ka != kb) return ka > kb;
            return a.label < b.label;
            });

        if ((int)suggestions.size() > m_MaxSuggestions) suggestions.resize(m_MaxSuggestions);

        selectedSuggestion = 0;
        showAutoComplete = !suggestions.empty();
    }

    void CodeEditor::InsertAutoComplete(const std::string& word)
    {
        auto [from, to] = GetCurrentWordRange();
        editor.SetSelection(from, to);
        editor.DeleteSelection();
        editor.InsertText(word);
    }

    std::pair<TextEditor::Coordinates, TextEditor::Coordinates> CodeEditor::GetCurrentWordRange() const
    {
        auto pos = editor.GetCursorPosition();
        std::string lineText = editor.GetCurrentLineText();
        int L = pos.mLine;
        int c0 = pos.mColumn, a = c0 - 1, b = c0;
        auto isWord = [](char ch) { return std::isalnum((unsigned char)ch) || ch == '_'; };
        while (a >= 0 && a < (int)lineText.size() && isWord(lineText[a])) --a;
        while (b >= 0 && b < (int)lineText.size() && isWord(lineText[b])) ++b;
        TextEditor::Coordinates from{ L, std::max(0, a + 1) };
        TextEditor::Coordinates to{ L, b };
        return { from,to };
    }

    static inline bool isBoundary(char p, char c) {
        return (std::islower((unsigned char)p) && std::isupper((unsigned char)c)) || (!std::isalnum((unsigned char)p) && std::isalnum((unsigned char)c));
    }

    int CodeEditor::FuzzyScore(const std::string& q, const std::string& t)
    {
        if (q.empty()) return 0;
        auto low = [](unsigned char ch) { return (char)std::tolower(ch); };
        int score = 0;
        int ti = 0;
        bool lastMatch = false;
        for (size_t qi = 0; qi < q.size(); ++qi) {
            char qc = low(q[qi]);
            bool found = false;
            for (; ti < (int)t.size(); ++ti) {
                char tc = low(t[ti]);
                if (qc == tc) {
                    int s = 5;
                    if (ti == 0) s += 25;
                    if (ti > 0 && isBoundary(t[ti - 1], t[ti])) s += 10;
                    if (lastMatch) s += 8;
                    score += s;
                    lastMatch = true;
                    ++ti;
                    found = true;
                    break;
                }
                else {
                    lastMatch = false;
                }
            }
            if (!found) return -100000;
        }
        if (t.size() >= q.size() && std::equal(q.begin(), q.end(), t.begin(), [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
            score += 40;
        }
        return score;
    }

    void CodeEditor::ProviderKeywords(const std::string& w, std::vector<Suggestion>& out)
    {
        if (w.empty()) return;
        for (auto& k : keywords) {
            int sc = FuzzyScore(w, k);
            if (sc > 0) {
                out.push_back({ k, k, SuggestionKind::Keyword, "keyword", "", sc + 5 });
            }
        }
    }

    void CodeEditor::ProviderDictionary(const std::string& w, std::vector<Suggestion>& out)
    {
        if (w.empty()) return;
        for (auto& s : dictionary) {
            int sc = FuzzyScore(w, s);
            if (sc > 0) {
                out.push_back({ s, s, SuggestionKind::Identifier, "dict", "", sc });
            }
        }
    }

    void CodeEditor::ProviderBufferIdentifiers(const std::string& w, std::vector<Suggestion>& out)
    {
        if (w.empty()) return;
        for (auto& s : bufferIdentifiers) {
            int sc = FuzzyScore(w, s);
            if (sc > 0) {
                out.push_back({ s, s, SuggestionKind::Identifier, "in-buffer", "", sc + 10 });
            }
        }
    }

    void CodeEditor::ProviderSnippets(const std::string& w, std::vector<Suggestion>& out)
    {
        if (w.empty()) return;
        for (auto& sn : snippetBank) {
            int sc = FuzzyScore(w, sn.label);
            if (sc > 0) {
                Suggestion s = sn;
                s.score = sc + 15;
                out.push_back(std::move(s));
            }
        }
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

    static const char* KindIcon(CodeEditor::SuggestionKind k) {
        switch (k) {
        case CodeEditor::SuggestionKind::Keyword:    return "[K]";
        case CodeEditor::SuggestionKind::Identifier: return "[ID]";
        case CodeEditor::SuggestionKind::Function:   return "[ƒ]";
        case CodeEditor::SuggestionKind::Snippet:    return "[S]";
        case CodeEditor::SuggestionKind::FilePath:   return "[F]";
        default: return "[?]";
        }
    }

    void CodeEditor::RenderAutoCompletePopup(const ImVec2& pos, const std::string& currentWord)
    {
        ImGui::SetNextWindowBgAlpha(0.98f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(240, 80), ImVec2(480, 360));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;

        if (ImGui::Begin("##AutoComplete", nullptr, flags))
        {
            ImGui::BeginChild("##list", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 3), true);
            for (int i = 0; i < (int)suggestions.size(); ++i) {
                const auto& s = suggestions[i];
                ImGui::PushID(i);
                bool sel = (i == selectedSuggestion);
                if (sel) ImGui::SetItemDefaultFocus();

                ImGui::Selectable((std::string(KindIcon(s.kind)) + " " + s.label).c_str(), sel);
                if (ImGui::IsItemHovered() && !s.doc.empty())
                    ImGui::SetTooltip("%s", s.doc.c_str());

                if (!s.detail.empty()) {
                    ImVec2 end = ImGui::GetItemRectMax();
                    float w = ImGui::CalcTextSize(s.detail.c_str()).x;
                    ImGui::SameLine(std::max(0.0f, ImGui::GetWindowContentRegionMax().x - w - 8.0f));
                    ImGui::TextUnformatted(s.detail.c_str());
                }

                if (ImGui::IsItemClicked()) {
                    InsertAutoComplete(s.insertText.empty() ? s.label : s.insertText);
                    showAutoComplete = false;
                    ImGui::PopID();
                    ImGui::EndChild();
                    ImGui::End();
                    return;
                }
                ImGui::PopID();
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::TextUnformatted("Enter/Tab: insérer   \xE2\x86\x91/\xE2\x86\x93: naviguer   Esc: fermer");

            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) selectedSuggestion = (selectedSuggestion + 1) % (int)suggestions.size();
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))   selectedSuggestion = (selectedSuggestion - 1 + (int)suggestions.size()) % (int)suggestions.size();
            if (ImGui::IsKeyPressed(ImGuiKey_PageDown))  selectedSuggestion = std::min((int)suggestions.size() - 1, selectedSuggestion + 8);
            if (ImGui::IsKeyPressed(ImGuiKey_PageUp))    selectedSuggestion = std::max(0, selectedSuggestion - 8);
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))    showAutoComplete = false;

            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Tab)) {
                auto s = suggestions[selectedSuggestion];
                InsertAutoComplete(s.insertText.empty() ? s.label : s.insertText);
                showAutoComplete = false;
                ImGui::End();
                return;
            }
        }
        ImGui::End();
    }

    void CodeEditor::OnImGuiRender(const char* id)
    {
        std::string windowTitle = "Code Editor";
        if (id && *id)
            windowTitle += "##" + std::string(id);

        if (ImGui::Begin(windowTitle.c_str()))
        {
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

            if (ImGui::Button("Find (Ctrl+F)")) searching = !searching;
            if (searching) RenderSearchBar();

            for (const auto& diag : diagnostics) {
                ImVec4 col = (diag.severity == TextEditor::PaletteIndex::ErrorMarker)
                    ? ImVec4(1, 0.3f, 0.3f, 1)
                    : ImVec4(1, 1, 0, 1);
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                if (ImGui::Selectable((std::to_string(diag.line + 1) + ": " + diag.message).c_str())) {
                    if (OnDiagnosticClicked) OnDiagnosticClicked(diag);
                    editor.SetCursorPosition({ diag.line, 0 });
                }
                ImGui::PopStyleColor();
            }

            ImGui::Separator();
            ImGui::Text("Zoom: %d%%", (int)std::round(m_Zoom * 100.f)); ImGui::SameLine();
            if (ImGui::SmallButton("-")) ZoomOut(); ImGui::SameLine();
            if (ImGui::SmallButton("+")) ZoomIn();  ImGui::SameLine();
            if (ImGui::SmallButton("Reset")) ResetZoom();
            ImGui::Separator();

            ImGui::PushID("EditorArea");
            ImGui::BeginChild("##EditorArea", ImVec2(0, 0), false, 0);

            ImGui::SetWindowFontScale(m_Zoom);

            ImGuiIO& io = ImGui::GetIO();
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
                if (io.KeyCtrl && io.MouseWheel != 0.0f) {
                    SetZoom(m_Zoom + io.MouseWheel * m_ZoomStep);
                }
                if (io.KeyCtrl) {
                    if (ImGui::IsKeyPressed(ImGuiKey_Equal, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd, false))
                        ZoomIn();
                    if (ImGui::IsKeyPressed(ImGuiKey_Minus, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract, false))
                        ZoomOut();
                    if (ImGui::IsKeyPressed(ImGuiKey_0, false))
                        ResetZoom();
                }
            }

            if (ImGui::IsKeyPressed(ImGuiKey_S, false) && io.KeyCtrl) {
                if (!currentFile.empty()) SaveToFile(currentFile);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F, false) && io.KeyCtrl) {
                searching = true;
            }

            editor.Render(id);

            if (font)
                ImGui::PopFont();

            if (editor.IsTextChanged()) {
                textChanged = true;
                RebuildBufferIdentifiers();
                if (OnTextChanged) OnTextChanged();
            }

            /*std::string currentWord = GetCurrentWord();

            bool forcePopup = (ImGui::IsKeyPressed(ImGuiKey_Space) && io.KeyCtrl);
            if (forcePopup || (!currentWord.empty())) {
                UpdateAutoCompleteSuggestions(currentWord);
            }

            if (showAutoComplete && !suggestions.empty()) {
                ImVec2 base = ImGui::GetWindowPos();
                ImVec2 scrMin = ImGui::GetWindowContentRegionMin();
                ImVec2 scrMax = ImGui::GetWindowContentRegionMax();
                ImVec2 pos = base + ImVec2(scrMin.x, scrMax.y);
                RenderAutoCompletePopup(pos, currentWord);
            }*/

            RenderContextMenu();

            ImGui::EndChild();
            ImGui::PopID();
        }
        ImGui::End();
    }

    void CodeEditor::RebuildBufferIdentifiers()
    {
        std::set<std::string> uniq;
        auto lines = editor.GetTextLines();
        for (auto& L : lines) {
            const std::string& s = L;
            std::string tok;
            for (size_t i = 0; i <= s.size(); ++i) {
                char c = (i < s.size() ? s[i] : ' ');
                if (std::isalnum((unsigned char)c) || c == '_') tok.push_back(c);
                else {
                    if (tok.size() >= 2) uniq.insert(tok);
                    tok.clear();
                }
            }
        }
        bufferIdentifiers.assign(uniq.begin(), uniq.end());
    }

    std::string CodeEditor::SliceTextFromRange(const TextEditor::Coordinates& coord_from, const TextEditor::Coordinates& coord_to) const
    {
        using Co = TextEditor::Coordinates;
        Co from = coord_from, to = coord_to;
        if (to < from) std::swap(from, to);

        const auto lines = editor.GetTextLines();
        if (lines.empty()) return {};

        auto clampLine = [&](int L) { return std::max(0, std::min((int)lines.size() - 1, L)); };
        int startL = clampLine(from.mLine);
        int endL = clampLine(to.mLine);

        std::string out;
        out.reserve(64);

        for (int L = startL; L <= endL; ++L) {
            const std::string& line = lines[L];
            int startC = (L == startL ? from.mColumn : 0);
            int endC = (L == endL ? to.mColumn : (int)line.size());

            startC = std::max(0, std::min((int)line.size(), startC));
            endC = std::max(0, std::min((int)line.size(), endC));

            if (endC > startC)
                out.append(line.substr(startC, endC - startC));
            if (L < endL) out.push_back('\n');
        }
        return out;
    }

}