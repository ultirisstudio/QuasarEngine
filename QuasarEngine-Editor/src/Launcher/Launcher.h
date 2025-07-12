#pragma once

#include <QuasarEngine.h>

#include "ProjectManager.h"
#include "yaml-cpp/yaml.h"

namespace QuasarEngine
{
	class Launcher : public Layer
	{
	public:
		Launcher();
		virtual ~Launcher() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(double dt) override;
		void OnGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		std::unique_ptr<ProjectManager> m_ProjectManager;
		YAML::Node config;
	};
}