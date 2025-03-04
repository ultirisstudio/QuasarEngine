#include "QtWindow.h"

#include <QuasarEngine/EngineFactory.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QtWindow::QtWindow() {

}

QtWindow::~QtWindow() {

}

void QtWindow::Initialize(unsigned int width, unsigned int height, const std::string& title) {

}

void QtWindow::SwapBuffers() {
    
}

void QtWindow::PollEvents() {
    
}

void QtWindow::Run() {
    
}

void QtWindow::Close() {
    
}

void QtWindow::Register()
{
    EngineFactory::Instance().RegisterWindow(WindowAPI::Qt, [&] {
        return std::make_unique<QtWindow>();
    });
}
