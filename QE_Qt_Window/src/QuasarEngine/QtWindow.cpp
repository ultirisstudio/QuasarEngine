#include "QtWindow.h"

#include <QuasarEngine/EngineFactory.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QtWindow::QtWindow(int width, int height, const char* title, int argc, char* argv[]) {
    /*app = new QApplication(argc, argv);
    window = new QWidget();
    window->resize(width, height);
    window->setWindowTitle(title);*/

    QApplication app(argc, argv);
    QWidget widget;
    widget.resize(800, 600);
    widget.setWindowTitle("Hello World");
    widget.show();
    app.exec();
}

QtWindow::~QtWindow() {
    //delete app;
    //delete window;
}

void QtWindow::Initialize() {
    //window->show();
}

void QtWindow::SwapBuffers() {
    
}

void QtWindow::PollEvents() {
    
}

void QtWindow::Run()
{
    //app->exec();
}

void QtWindow::Close() {
    //window->close();
}

void QtWindow::Register(int argc, char* argv[])
{
    EngineFactory::Instance().RegisterWindow(WindowAPI::Qt, [&] {
        return std::make_unique<QtWindow>(800, 600, "Qt Window", argc, argv);
    });
}
