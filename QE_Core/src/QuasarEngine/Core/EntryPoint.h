#pragma once

#include <QuasarEngine/Core/Core.h>

extern Application* CreateApplication(ApplicationCommandLineArgs args);

int main(int argc, char** argv)
{
	auto app = CreateApplication({ argc, argv });
	app->Run();
	delete app;
}
