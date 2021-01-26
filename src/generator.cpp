#include "ImgGenerator.h"


//main entry
int main(int argc, char ** argv) {

    osg::ArgumentParser arguments(&argc,argv);
    std::string cfgPath;
    bool hasConfig = arguments.read("-cfg", cfgPath);
    if (!hasConfig) {
        cfgPath = "./config.json";
    }
    Configurator cfg;
    cfg.parse(cfgPath);
    int mode = 0;
    arguments.read("-mode", mode);
    if (mode == 2) { //debug mode, configuration check
        std::vector<Translation> tList = cfg.getTranslations();
        osg::notify(osg::NOTICE)<<"Translations "<<tList.size()<<std::endl;
        if (tList.size() > 0) {
            Translation t = tList[0];
            osg::notify(osg::NOTICE)<<"T random "<<t.random<<", positon x "<<t.position.x_from<<" : "<<t.position.x_to<<std::endl;
            osg::notify(osg::NOTICE)<<"T angle x "<<t.angle.x_from<<" : "<<t.angle.x_to<<", multiSamples "<<cfg.getOutput().numMultiSamples<<std::endl;
        }
    }
    else {
        ImgGenerator* generator = new ImgGenerator(cfg);
        generator->setMode(mode);
        if (mode == 4) {
            generator->generateMultipleImages();
        }
        else {
            generator->generateImages();
        }
        delete generator;
    }
    return 0;
}

