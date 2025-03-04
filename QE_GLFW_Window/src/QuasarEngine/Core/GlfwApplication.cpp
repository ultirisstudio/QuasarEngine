#include "GlfwApplication.h"

GlfwApplication::GlfwApplication(const ApplicationSpecification& specification) : Application(specification), m_GlfwWindow(dynamic_cast<GlfwWindow*>(m_Window.get()))
{
	s_Instance = this;
}

GlfwApplication::~GlfwApplication()
{

}

void GlfwApplication::Run()
{

}
