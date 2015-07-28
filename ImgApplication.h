#ifndef IMG_APPLICATION
#define IMG_APPLICATION

#include "qapplication.h"
#include "imagine.h"
#include "camera_g.hpp"
#include "positioner.hpp"

/*
This is meant to be the top-level object of this application.
It's a good place to keep things that might otherwise be globals.
*/
class ImgApplication :
    public QApplication
{

public:
    ImgApplication::ImgApplication(int &argc, char **argv);
    ImgApplication::~ImgApplication();

    // the main ui objects
    Imagine *imgOne = NULL;
    Imagine *imgTwo = NULL;

    // Initialize the main window(s)
    void ImgApplication::initUI(Camera *cam1, Positioner *pos, Camera *cam2 = nullptr);

    // Show the main window(s).
    void ImgApplication::showUi();
};

#endif