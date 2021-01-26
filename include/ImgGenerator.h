#ifndef IMGGENERATOR_H
#define IMGGENERATOR_H

#include <Configurator.h>
#include "SaveImageCallback.h"

#include <osgViewer/Viewer>
#include <osg/Node>
#include <osg/Camera>
#include <osg/Texture>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>
#include <osg/DisplaySettings>
#include <osg/Math>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>

/**
    The class contains set of methods to generate collection of images according to given configuration.
    3D models used as base, specified translations (position, rotation, scale) applied to 3D objects,
    then 2D image generated.
*/
class ImgGenerator
{
    public:
        ImgGenerator(Configurator& cfg);
        virtual ~ImgGenerator();
        int generateImages();
        int generateMultipleImages();
        void setMode(int _mode) {mode = _mode;}
    protected:
        osg::ref_ptr<osg::Camera> createBackgroundCamera();
        osg::ref_ptr<osg::TextureRectangle> createBackgroundTexture(osg::Camera* bg_cam, float s, float t);
        void generateImage(osg::Image* image, std::string fileName,
            osg::ref_ptr<osg::TextureRectangle> &textureRect, osgViewer::Viewer &viewer);
        void setTranslation(osg::ref_ptr<osg::PositionAttitudeTransform> modelTf, Translation tr, int j,
                std::string &labels, std::string fileShortName);
        void setTranslation(std::vector<osg::ref_ptr<osg::PositionAttitudeTransform> > transforms, Translation tr, int j,
                std::string &labels, std::string fileShortName, int groupCount);
        int makeDir(std::string path, std::string name);
        int makeDir(std::string path);
        bool dirExists(std::string dir);
        void createInfo(std::string path, std::string content);
        void createLabels(std::string path, std::string content);

        osg::Vec3d getPosition(Translation tr, int j);
        osg::Vec3d getRotation(Translation tr, int j);
        osg::Vec3d getScale(Translation tr, int j);
        double getRand(double min, double max);
        osg::Image* cropImage(const osg::Image* image,
                      double src_minx, double src_miny, double src_maxx, double src_maxy,
                      double &dst_minx, double &dst_miny, double &dst_maxx, double &dst_maxy);

        void setGroupShift(std::vector<osg::ref_ptr<osg::PositionAttitudeTransform> > transforms, osg::Vec3d position, int groupCount);
        void setGroupShift(osg::ref_ptr<osg::PositionAttitudeTransform> tr,
                osg::ref_ptr<osg::PositionAttitudeTransform> tr2,
                osg::Vec3d position, int signX, int signY, int signZ);
        void initTraits(int bgWidth, int bgHeight, osgViewer::Viewer &viewer);
    private:
        Configurator config;
        //mode = 0 - view (default), mode = 1 - generate.
        int mode;
};

#endif // IMGGENERATOR_H
