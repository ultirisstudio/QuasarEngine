#pragma once

#include "QuasarEngine/Core/Core.h"

//#include <memory>
//#include <unordered_map>
//#include <glm/glm.hpp>

namespace QuasarEngine
{
	extern Application* CreateApplication(ApplicationCommandLineArgs args);
}

int main(int argc, char** argv)
{
	std::unique_ptr<QuasarEngine::Application> app(QuasarEngine::CreateApplication({ argc, argv }));
	app->Run();

	return 0;
}