#include "GlfwApplication.h"

GlfwApplication::GlfwApplication(const ApplicationSpecification& specification) : Application(specification)
{
	s_Instance = this;
}

GlfwApplication::~GlfwApplication()
{

}

void GlfwApplication::Run()
{

}
