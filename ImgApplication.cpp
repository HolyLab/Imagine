#include "ImgApplication.h"


#pragma region LIFECYCLE

ImgApplication::ImgApplication(int &argc, char **argv) : QApplication(argc, argv){

}


ImgApplication::~ImgApplication() {
    if (imgOne != NULL) delete(imgOne);
    if (imgTwo != NULL) delete(imgTwo);
}

#pragma endregion

#pragma region UI

void ImgApplication::showUi() {
    if (imgOne != NULL) imgOne->show();
    if (imgTwo != NULL) imgTwo->show();
}

void ImgApplication::initUI(Camera *cam1, Positioner *pos, Camera *cam2) {
    // init first window
    Imagine *w1p = new Imagine(cam1, pos);
    imgOne = w1p;

    // init second window if needed
    Imagine *w2p = nullptr;
    if (cam2 != NULL) {
        w2p = new Imagine(cam2, nullptr);
        imgTwo = w2p;
        imgTwo->setWindowTitle("Imagine (2)");
        imgOne->setWindowTitle("Imagine (1)");
    }
}

#pragma endregion