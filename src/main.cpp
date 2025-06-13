#include "application.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    Application application(argv[1]);
    application.show();

    int result = app.exec();

    stop_exiftool();
    
    return result;
}
