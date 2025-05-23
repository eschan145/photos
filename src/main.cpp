#include "application.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    Application application;
    application.show();

    int result = app.exec();

    stop_exiftool();
    
    return result;
}
