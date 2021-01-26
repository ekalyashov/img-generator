#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include <string>
#include <vector>
#include <map>

#include <osgDB/ReadFile>

struct Bounds {
    float x_from;
    float x_to;
    float x_delta;
    float y_from;
    float y_to;
    float y_delta;
    float z_from;
    float z_to;
    float z_delta;
};

struct Output {
    int width;
    int height;
    std::string folder;
    std::string extension;
    int numMultiSamples;
    std::string maskFolder;
    int numObjects;
    //min and max shifts between objects
    Bounds objShifts;
};

struct Translation {
    int count;
    Bounds position;
    Bounds angle;
    float scale_from;
    float scale_to;
    float scale_delta;
    bool random;

    void prepare() {
        position.x_delta = (position.x_to - position.x_from) / count;
        position.y_delta = (position.y_to - position.y_from) / count;
        position.z_delta = (position.z_to - position.z_from) / count;
        angle.x_delta = (angle.x_to - angle.x_from) / count;
        angle.y_delta = (angle.y_to - angle.y_from) / count;
        angle.z_delta = (angle.z_to - angle.z_from) / count;
        scale_delta = (scale_to - scale_from) / count;
    }
};

/**
    This class ensures loading of configuration and utility methods to load necessary objects.
*/
class Configurator {
    public:
        Configurator();
        virtual ~Configurator();
        int parse(const std::string &filename);
        int findFiles(const std::string &dir, const std::vector<std::string> &extensions, std::vector<std::string> &result);
        std::vector<std::string> getBackgroundFiles() {return bg_files;}
        std::vector<std::string> getModelFiles() {return model_files;}
        const std::vector<Translation> &getTranslations() const {return translations;}
        std::vector<osg::Image*> loadBackground();
        osg::Image* loadMaskBackground();
        std::map<std::string, osg::Node*> loadModels();
        Output getOutput() {return output;}
        std::string getAsString() {return jsonString;}
        bool bgAugmentation() {return doBgAugmentation;}
    protected:
        bool hasExtension(const std::string &fileName, const std::string &ext);
    private:
        std::vector<std::string> bg_files;
        std::vector<std::string> model_files;
        std::string mask_bg_file;
        Output output;
        std::vector<Translation> translations;
        std::string jsonString;
        bool doBgAugmentation;
};

#endif // CONFIGURATOR_H
