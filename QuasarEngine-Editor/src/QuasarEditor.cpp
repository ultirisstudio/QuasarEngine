#include <QuasarEngine.h>
#include <QuasarEngine/Core/EntryPoint.h>

#include "Editor/Editor.h"
#include "Launcher/Launcher.h"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <filesystem>
#include <optional>
#include <string>
#include <sstream>

namespace QuasarEngine
{
    static void EnsureConfigExists()
    {
        if (std::filesystem::exists("config.ultconf"))
            return;

        YAML::Node config;
        config["projectName"] = "";
        config["projectPath"] = "";
        config["recentProjects"] = YAML::Node(YAML::NodeType::Sequence);
        config["settings"]["autoOpenLastProject"] = false;
        config["settings"]["showMissingProjects"] = true;
        config["settings"]["compactCards"] = false;

        std::ofstream fout("config.ultconf");
        fout << config;
    }

    static std::string TrimQuotes(std::string s)
    {
        if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')))
            return s.substr(1, s.size() - 2);
        return s;
    }

    static std::optional<std::string> GetProjectArg(const ApplicationCommandLineArgs& args)
    {
        for (int i = 1; i < args.Count; i++)
        {
            std::string a = args[i];

            const std::string prefix = "--project=";
            if (a.rfind(prefix, 0) == 0)
            {
                std::string v = a.substr(prefix.size());
                v = TrimQuotes(v);
                if (!v.empty())
                    return v;
            }

            if (a == "--project" && i + 1 < args.Count)
            {
                std::string v = TrimQuotes(std::string(args[i + 1]));
                if (!v.empty())
                    return v;
            }
        }

        return std::nullopt;
    }

    static std::string ReadProjectNameFromUltprj(const std::string& projectPath)
    {
        std::string folderName = std::filesystem::path(projectPath).filename().string();
        std::string prjFile = projectPath + "\\" + folderName + ".ultprj";

        if (!std::filesystem::exists(prjFile))
            return folderName;

        try
        {
            std::ifstream stream(prjFile);
            std::stringstream ss;
            ss << stream.rdbuf();
            YAML::Node data = YAML::Load(ss.str());
            if (data["Project"])
                return data["Project"].as<std::string>();
        }
        catch (...) {}

        return folderName;
    }

    class QuasarEditor : public Application
    {
    public:
        QuasarEditor(const ApplicationSpecification& spec) : Application(spec)
        {
            EditorSpecification editorSpec;
            editorSpec.EngineExecutablePath = spec.CommandLineArgs[0];
            editorSpec.ProjectPath = "C:\\Users\\rouff\\Documents\\Ultiris Projects\\MyProject";
            editorSpec.ProjectName = "MyProject";

            PushLayer(new Editor(editorSpec));

            return;

            EnsureConfigExists();

            if (auto projectArg = GetProjectArg(spec.CommandLineArgs))
            {
                const std::string projectPath = *projectArg;

                if (std::filesystem::exists(projectPath))
                {
                    EditorSpecification editorSpec;
                    editorSpec.EngineExecutablePath = spec.CommandLineArgs[0];
                    editorSpec.ProjectPath = projectPath;
                    editorSpec.ProjectName = ReadProjectNameFromUltprj(projectPath);

                    PushLayer(new Editor(editorSpec));
                    return;
                }

                PushLayer(new Launcher());
                return;
            }

            PushLayer(new Launcher());
        }

        ~QuasarEditor() override = default;
    };

    Application* CreateApplication(ApplicationCommandLineArgs args)
    {
        ApplicationSpecification spec;
        spec.Name = "Quasar Editor";
        spec.CommandLineArgs = args;

        spec.EventMode = QuasarEngine::ApplicationSpecification::EventPumpMode::Poll;
        spec.EventWaitTimeoutSec = 0.0;
        spec.ImGuiMaxFPS = 0;
        spec.MaxFPS = 0;

        return new QuasarEditor(spec);
    }
}
