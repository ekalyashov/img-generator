#include "Configurator.h"
#include <json/json.h>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
#include <string.h>

//constructor
Configurator::Configurator() {
    //initialize
    output.width = 800;
    output.numObjects = 1;
}

//destructor
Configurator::~Configurator() {

}

/**
    Parsing of configuration file and filling Configurator fields
    @param filename path to configuration file.
    @return 0 on success, or 1 if error occur
*/
int Configurator::parse(const std::string &filename) {
    std::ifstream fin(filename.c_str());
    if(fin.fail()) {
        std::cout  << "Config file do not exists or access denied\n";
        return 1;
    }

    Json::Reader reader;
    Json::Value config;
    bool success = reader.parse(fin, config);

    if (!success)
    {
        std::cout  << "Failed to parse configuration\n"  << reader.getFormattedErrorMessages();
        return 1;
    }
    Json::StyledWriter writer;
    jsonString = writer.write(config);
    Json::Value generator = config["generator"];
    std::string bg_folder = generator["input"]["background_folder"].asString();
    std::string model_folder = generator["input"]["model_folder"].asString();
    mask_bg_file = generator["input"]["mask_background"].asString();
    doBgAugmentation = generator["input"]["bg_augmentation"].asBool();
    output.width = generator["output"]["size"]["width"].asInt();
    output.height = generator["output"]["size"]["height"].asInt();
    output.folder = generator["output"]["output_folder"].asString();
    output.extension = generator["output"]["extension"].asString();
    output.numMultiSamples = generator["output"]["num_multi_samples"].asInt();
    output.maskFolder = generator["output"]["mask_folder"].asString();
    output.numObjects = generator["output"]["num_objects"].asInt();
    if (output.numObjects == 0) {
        output.numObjects = 1;
    }
    Json::Value shift = generator["output"]["obj_shifts"];
    output.objShifts.x_from = shift["x"].asFloat();
    output.objShifts.x_to = shift["x_to"].asFloat();
    output.objShifts.y_from = shift["y"].asFloat();
    output.objShifts.y_to = shift["y_to"].asFloat();
    output.objShifts.z_from = shift["z"].asFloat();
    output.objShifts.z_to = shift["z_to"].asFloat();

    Json::Value trans_list = generator["translations"];
    translations.clear();
    for (int i=0; i < trans_list.size(); i++) {
        Json::Value t = trans_list[i];
        Translation trans;
        trans.count = t["count"].asInt();
        trans.random = t["random"].asBool();
        trans.position.x_from = t["position"]["x"]["from"].asFloat();
        trans.position.x_to = t["position"]["x"]["to"].asFloat();
        trans.position.y_from = t["position"]["y"]["from"].asFloat();
        trans.position.y_to = t["position"]["y"]["to"].asFloat();
        trans.position.z_from = t["position"]["z"]["from"].asFloat();
        trans.position.z_to = t["position"]["z"]["to"].asFloat();

        trans.angle.x_from = t["angle"]["x"]["from"].asFloat();
        trans.angle.x_to = t["angle"]["x"]["to"].asFloat();
        trans.angle.y_from = t["angle"]["y"]["from"].asFloat();
        trans.angle.y_to = t["angle"]["y"]["to"].asFloat();
        trans.angle.z_from = t["angle"]["z"]["from"].asFloat();
        trans.angle.z_to = t["angle"]["z"]["to"].asFloat();

        trans.scale_from = t["scale"]["from"].asFloat();
        trans.scale_to = t["scale"]["to"].asFloat();
        trans.prepare();

        translations.push_back(trans);
    }
    std::cout  << "translation size="<<translations.size()<<std::endl;

    std::vector<std::string> bg_extensions;
    bg_extensions.push_back(".jpg");
    bg_extensions.push_back(".png");
    bg_extensions.push_back(".JPG");
    bg_extensions.push_back(".PNG");
    bg_files.clear();
    findFiles(bg_folder, bg_extensions, bg_files);
    std::vector<std::string> model_extensions;
    model_extensions.push_back(".obj");
    model_extensions.push_back(".OBJ");
    model_extensions.push_back(".3ds");
    model_extensions.push_back(".3DS");
    model_files.clear();
    return findFiles(model_folder, model_extensions, model_files);
}

/**
    Recursively searches files in specified directory, filters with given extensions.
    @param dir initial directory
    @param extensions list of allowed extensions
    @param result output vector with paths of found files
    @return  0 if success
*/
int Configurator::findFiles(const std::string &dir, const std::vector<std::string> &extensions, std::vector<std::string> &result) {
	DIR *tDir;
	tDir = opendir(dir.c_str());
	if(tDir == NULL) {
		std::cerr << std::endl << "Error opening directory " << dir << std::endl;
		return 1;
	}
	struct dirent *dirP;
	struct stat filestat;
	std::string path;
	while( (dirP = readdir(tDir)) ) {
		//Skip current object if it is this directory or parent directory
		if(!strncmp(dirP->d_name, ".", 1) || !strncmp(dirP->d_name, "..", 2)) {
			continue;
        }
		if(dir==".") {
            path = dirP->d_name;
        }
		else {
            path = dir + "/" + dirP->d_name;
        }
		//Skip current file / directory if it is invalid in some way
		if(stat(path.c_str(), &filestat)) continue;
		//Recursively call this function if current object is a directory
		if(S_ISDIR(filestat.st_mode)) {
			findFiles(path, extensions, result);
			continue;
		}
		else {
            for (int i=0; i < extensions.size(); i++) {
                std::string ext = extensions[i];
                if (hasExtension(path, ext)) {
                    result.push_back(path);
                    break;
                }
            }
		}
    }
	closedir(tDir);
	return 0;
}

/**
    Checks if specified file has valid extension.
    @return true if success false otherwise
*/
bool Configurator::hasExtension(const std::string &fileName, const std::string &ext)
{
    return fileName.size() >= ext.size() &&
           fileName.compare(fileName.size() - ext.size(), ext.size(), ext) == 0;
}

/**
    Loads list of background images.
    @return vector of osg::Image objects.
*/
std::vector<osg::Image*> Configurator::loadBackground() {
    int bgWidth = 800;
    int bgHeight = bgWidth * getOutput().height / getOutput().width;
    std::vector<osg::Image*> bgImages;
    for (int i = 0; i < bg_files.size(); i++) {
        std::string filename = bg_files[i];
        osg::Image* image = osgDB::readImageFile (filename);
        if (!image) {
            osg::notify(osg::NOTICE)<<"Background image file '"<<filename<<"' not found"<<std::endl;
        }
        image->scaleImage(bgWidth, bgHeight, image->r());
        bgImages.push_back(image);
    }
    return bgImages;
}

/**
    Loads background image for mask generation.
    @return osg::Image object.
*/
osg::Image* Configurator::loadMaskBackground() {
    int bgWidth = 800;
    int bgHeight = bgWidth * getOutput().height / getOutput().width;
    osg::Image* image = osgDB::readImageFile (mask_bg_file);
    if (!image) {
        osg::notify(osg::NOTICE)<<"Mask background image file '"<<mask_bg_file<<"' not found"<<std::endl;
    }
    image->scaleImage(bgWidth, bgHeight, image->r());
    return image;
}

/**
    Loads list of 3D models
    @return map containing loaded models as osg::Node objects
*/
std::map<std::string, osg::Node*> Configurator::loadModels() {
    std::map<std::string, osg::Node*> models;
    std::vector<std::string> fileList;
    for (int i = 0; i < model_files.size(); i++) {
        std::string filename = model_files[i];
        fileList.push_back(filename);
        osg::Node* model = osgDB::readNodeFiles(fileList);
        fileList.clear();
        if (model) {
            models[filename] = model;
        }
    }
    return models;
}
