#include "MainWindow.h"
#include "qhotkey.h"
#include <QtWidgets/QApplication>
#include <Windows.h>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow* w = new MainWindow;
    QHotkey* hotkey = new QHotkey(MOD_ALT, VK_SPACE);
    QHotkey::connect(hotkey, &QHotkey::pressed, w, [=]() {
        if (w->isHidden()) {
            w->show();
        }
        else {
            w->hide();
        }
        });
    int ret = a.exec();
    delete hotkey;
    delete w;
    return ret;
}
