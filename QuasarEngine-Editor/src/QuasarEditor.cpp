#include <QuasarEngine.h>
#include <QuasarEngine/Core/EntryPoint.h>

#include "Editor/Editor.h"
#include "Launcher/Launcher.h"
#include "yaml-cpp/yaml.h"
#include <fstream>

namespace QuasarEngine
{
	class QuasarEditor : public Application
	{
	public:
		QuasarEditor(const ApplicationSpecification& spec) : Application(spec)
		{
			if (std::filesystem::exists("config.ultconf"))
			{
				YAML::Node config = YAML::LoadFile("config.ultconf");
				if (config["projectName"] && config["projectPath"])
				{
					if (config["projectName"].as<std::string>() == "" || config["projectPath"].as<std::string>() == "")
					{
						PushLayer(new Launcher());
						return;
					}
					if (!std::filesystem::exists(config["projectPath"].as<std::string>()))
					{
						PushLayer(new Launcher());
						return;
					}
					EditorSpecification editorSpec;
					editorSpec.EngineExecutablePath = spec.CommandLineArgs[0];
					editorSpec.ProjectName = config["projectName"].as<std::string>();
					editorSpec.ProjectPath = config["projectPath"].as<std::string>();

					PushLayer(new Editor(editorSpec));
				}
				else
				{
					PushLayer(new Launcher());
				}
			}
			else
			{
				YAML::Node config;
				config["projectName"] = "";
				config["projectPath"] = "";
				std::ofstream fout("config.ultconf");
				fout << config;
				fout.close();

				PushLayer(new Launcher());
			}
		}

		~QuasarEditor()
		{

		}
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