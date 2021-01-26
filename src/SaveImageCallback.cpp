#include <osgDB/WriteFile>
#include "SaveImageCallback.h"

//constructor
SaveImageCallback::SaveImageCallback(int _outputWidth) {
    outputWidth = _outputWidth;
    finished = true;
}

//destructor
SaveImageCallback::~SaveImageCallback() {

}

/**
    Overrided operator (), saves generated image to a file
    @param bg_cam osg Camera.
*/
void SaveImageCallback::operator () (const osg::Camera& camera) const {
    finished = false;
    int x,y,width,height;
    x = camera.getViewport()->x();
    y = camera.getViewport()->y();
    width = camera.getViewport()->width();
    height = camera.getViewport()->height();

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->readPixels(x,y,width,height,GL_RGB,GL_UNSIGNED_BYTE);
    int w = width - x;
    int h = height - y;
    int t = outputWidth * h / w;
    image->scaleImage(outputWidth, t, 1);

    if (osgDB::writeImageFile(*image, fileName)) {
        std::cout << "Saved screen image to `"<<fileName<<"`"<< std::endl;
    }
    finished = true;
}
