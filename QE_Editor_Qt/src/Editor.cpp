#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    QWidget window;
    window.resize(400, 300);
    window.setWindowTitle("Ma fenêtre Qt");
    window.show();

    return a.exec();
}