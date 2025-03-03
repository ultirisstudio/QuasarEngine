#include "QtWindow.h"

#include <QuasarEngine/EngineFactory.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QtWindow::QtWindow(int width, int height, const char* title) {

}

QtWindow::~QtWindow() {

}

void QtWindow::Initialize() {

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
        return std::make_unique<QtWindow>(800, 600, "Qt Window");
    });
}
