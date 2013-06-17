#include "main_window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    main_window w;
    //w.showFullScreen();
    w.show();
    return app.exec();
}
