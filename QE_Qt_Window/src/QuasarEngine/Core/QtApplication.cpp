#include "QtApplication.h"

#include <QtWidgets/QWidget>

QtApplication::QtApplication(const ApplicationSpecification& specification) : Application(specification)
{
	s_Instance = this;
}

QtApplication::~QtApplication()
{

}

void QtApplication::Run()
{

}
