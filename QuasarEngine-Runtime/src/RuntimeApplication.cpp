#include <QuasarEngine.h>
#include <QuasarEngine/Core/EntryPoint.h>

#include "Runtime/Runtime.h"

namespace QuasarEngine
{
	class RuntimeApplication : public Application
	{
	public:
		RuntimeApplication(const ApplicationSpecification& spec) : Application(spec)
		{
			PushLayer(new Runtime());
		}

		~RuntimeApplication()
		{

		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Runtime";
		spec.CommandLineArgs = args;
		spec.EnableImGui = true;

		return new RuntimeApplication(spec);
	}
}