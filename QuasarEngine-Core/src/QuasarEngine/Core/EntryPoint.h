#pragma once

#include "QuasarEngine/Core/Core.h"

namespace QuasarEngine
{
	extern Application* CreateApplication(ApplicationCommandLineArgs args);
}

#ifdef PLATFORM_WINDOWS
#include <Windows.h>
#endif

int main(int argc, char** argv)
{
	std::unique_ptr<QuasarEngine::Application> app(QuasarEngine::CreateApplication({ argc, argv }));
	app->Run();

	return 0;
}