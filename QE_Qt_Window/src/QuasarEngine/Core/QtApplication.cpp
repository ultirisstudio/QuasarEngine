#include "QtApplication.h"

#include <QtWidgets/QWidget>

QtApplication::QtApplication(const ApplicationSpecification& specification) : Application(specification), m_QtWindow(dynamic_cast<QtWindow*>(m_Window.get()))
{
	s_Instance = this;
}

QtApplication::~QtApplication()
{

}

void QtApplication::Run()
{

}
