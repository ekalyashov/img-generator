#include "ImgGenerator.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <map>
#include <memory.h>

//constructor
ImgGenerator::ImgGenerator(Configurator& cfg) {
    config = cfg;
    srand( time( 0 ) );
}

//destructor
ImgGenerator::~ImgGenerator() {

}

//Utility method, number to string conversion
template <typename T>
std::string NumberToString(T pNumber) {
 std::ostringstream oOStrStream;
 oOStrStream << pNumber;
 return oOStrStream.str();
}

/**
    Number to string conversion method, with specified stringwidth
    @param num the integer to convert into string.
    @param width the width of result string, if needed a string filled with leading zeros.
    @return a string representation of int
*/
std::string intToString(int num, int width) {
    std::stringstream ss;
    ss << std::setw(width) << std::setfill('0') << num;
    return ss.str();
}

//utitity method, calculates length of string using specified base
//length of string should be enough to enumerate 'base' count of objects
int getFolderWidth10(int base) {
    //base-1 : zero-based, for 10 width=1 (0..9)
    return floor(log10(base - 1)) + 1;
}

int getWidth10(int base) {
    return floor(log10(base)) + 1;
}

//returns a pseudo-random integer between specified min and max values
int randint(int min, int max) {
    return min + (rand() % static_cast<int>(max - min + 1));
}

//Creates a new directory, using specified path and directory name.
int ImgGenerator::makeDir(std::string path, std::string name) {
    std::string command = "mkdir -p " + path + "/" + name;
    const int dir_err = system(command.c_str());
    return dir_err;
}

//Creates a new directory, using specified full path.
int ImgGenerator::makeDir(std::string path) {
    std::string command = "mkdir -p " + path;
    const int dir_err = system(command.c_str());
    return dir_err;
}

//Tests whether the file denoted by specified path exists and is directory
bool ImgGenerator::dirExists(std::string dir) {
    struct stat sb;
    return (stat(dir.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
}

/**
    Creates a new empty info file 'info.txt', using specified path, and writes specified content.
    @param path the path to info file.
    @param content the string to store into info file
*/
void ImgGenerator::createInfo(std::string path, std::string content) {
    std::string fileName = path + "/info.txt";
    std::ofstream outfile(fileName.c_str());
    outfile << content << std::endl;
    outfile.close();
}

/**
    Creates a new empty info file 'labels.csv', using specified path, and writes specified content.
    @param path the path to labels file.
    @param content the string to store into labels file
*/
void ImgGenerator::createLabels(std::string path, std::string content) {
    std::string fileName = path + "/labels.csv";
    std::ofstream outfile(fileName.c_str());
    outfile << content << std::endl;
    outfile.close();
}

/**
    Creates camera to present a background texture
    @return pointer to osg Camera
*/
osg::ref_ptr<osg::Camera> ImgGenerator::createBackgroundCamera() {
    osg::ref_ptr<osg::Camera> bg_cam = new osg::Camera();
    bg_cam->setRenderOrder(osg::Camera::PRE_RENDER);
    // set the view matrix
    bg_cam->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    // use identity view matrix so that children do not get (view) transformed
    bg_cam->setViewMatrix(osg::Matrix::identity());
    // set the projection matrix to be of width and height of 1
    bg_cam->setProjectionMatrix(osg::Matrix::ortho2D(0, 1.0f, 0, 1.0f));
    // set resize policy to fixed
    bg_cam->setProjectionResizePolicy(osg::Camera::FIXED);
    // only clear the depth buffer
    bg_cam->setClearMask(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    // we don't want the camera to grab event focus from the viewers main camera(s).
    bg_cam->setAllowEventFocus(false);

    osg::StateSet* cameraStateSet = bg_cam->getOrCreateStateSet();
    cameraStateSet->setRenderBinDetails(1, "RenderBin");
    cameraStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    return bg_cam;
}

/**
    Creates texture to draw background image.
    Texture coords go from bottom left (0,0) to right top (r,t).
    @param bg_cam osg Camera.
    @param s the float top tex coord.
    @param t the float right tex coord.
    @return pointer to background TextureRectangle
*/
osg::ref_ptr<osg::TextureRectangle> ImgGenerator::createBackgroundTexture(osg::Camera* bg_cam, float r, float t) {
    osg::Geode* pGeode = new osg::Geode();
    osg::StateSet* pStateSet = pGeode->getOrCreateStateSet();
    pStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    osg::Geometry* texturedQuad = osg::createTexturedQuadGeometry(
            osg::Vec3(0.f, 0.f, 0.f),
            osg::Vec3(1.0f, 0.f, 0.f),
            osg::Vec3(0.f, 1.0f, 0.f),
            0.f,
            0.f,
            r,
            t);
    osg::ref_ptr<osg::TextureRectangle> textureRect = new osg::TextureRectangle();
    textureRect->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    textureRect->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    textureRect->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    textureRect->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    texturedQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0, textureRect, osg::StateAttribute::ON);
    texturedQuad->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    pGeode->addDrawable(texturedQuad);
    bg_cam->addChild(pGeode);
    return textureRect;
}

/**
    Fills traits, creates GraphicsContext and initializes camera of specified viewer.
    @param bgWidth the int background width.
    @param bgHeight the int background height.
    @param viewer osg Viewer.
*/
void ImgGenerator::initTraits(int bgWidth, int bgHeight, osgViewer::Viewer &viewer) {
    int xoffset = 0;
    int yoffset = 0;
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = xoffset + 0;
    traits->y = yoffset + 0;
    traits->width = bgWidth;
    traits->height = bgHeight;
    std::cout << "traits: " << traits->x << ", "<<traits->y <<", "<<traits->width<<", "<<traits->height<< std::endl;
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;
    traits->samples = config.getOutput().numMultiSamples;

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer.getCamera()->setGraphicsContext(gc.get());
    viewer.getCamera()->setViewport(new osg::Viewport(0,0, traits->width, traits->height));
    GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
    viewer.getCamera()->setDrawBuffer(buffer);
    viewer.getCamera()->setReadBuffer(buffer);
}

/**
    Inner class implements osg NodeCallback
    Used as cull callbacks to ignore some nodes when draw image.
*/
class CullCallback : public osg::NodeCallback {
  public:
    CullCallback(): mEnabled(true) {
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {
        if (mEnabled) {
            return;
        }
        traverse(node, nv);
    }

    void setEnabled(bool enabled) {
        mEnabled = enabled;
    }

    bool isEnabled() {
        return mEnabled;
    }

  private:
    bool mEnabled;
};

/**
    Generates a set of images and correspondent masks according to defined configuration.
    Multiple objects can be painted in one image using multiple models.
    @return 0 on success, or 1 if error occur
*/
int ImgGenerator::generateMultipleImages() {
    std::vector<osg::Image*> bgImages = config.loadBackground();
    if (bgImages.size() == 0) {
        osg::notify(osg::NOTICE)<<"No background images found"<<std::endl;
        return 1;
    }
    std::map<std::string, osg::Node*> models = config.loadModels();
    if (models.size() == 0) {
        osg::notify(osg::NOTICE)<<"No models found"<<std::endl;
        return 1;
    }
    osg::Image* maskBgImage = config.loadMaskBackground();
    //check output folder
    int bgWidth = bgImages[0]->s();
    int bgHeight = bgImages[0]->t();
    osg::ref_ptr<osg::Camera> bg_cam = createBackgroundCamera();
    osg::ref_ptr<osg::TextureRectangle> textureRect =
        createBackgroundTexture(bg_cam.get(), bgWidth, bgHeight);
    //multisamles antialiasing
    osg::DisplaySettings::instance()->setNumMultiSamples(config.getOutput().numMultiSamples);
    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Group> root = new osg::Group();
    osg::ref_ptr<osg::PositionAttitudeTransform> oldModelTf = new osg::PositionAttitudeTransform();
    root->addChild(oldModelTf);
    root->addChild(bg_cam.get());

    viewer.setSceneData(root);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);

    viewer.getCamera()->setClearMask(GL_DEPTH_BUFFER_BIT);

    initTraits(bgWidth, bgHeight, viewer);

    osg::ref_ptr<SaveImageCallback> saveImageCallback = new SaveImageCallback(config.getOutput().width);
    viewer.getCamera()->setFinalDrawCallback(saveImageCallback.get());
    viewer.realize();

    std::vector<osg::ref_ptr<CullCallback> > cullCallbacks;
    std::vector<Translation> translations = config.getTranslations();
    std::vector<osg::ref_ptr<osg::PositionAttitudeTransform> > transforms;
    Output o = config.getOutput();
    std::string folderName = o.folder;
    if (!dirExists(folderName)) {
        makeDir(folderName);
    }
    if (!dirExists(o.maskFolder)) {
        makeDir(o.maskFolder);
    }
    osg::Vec4 ambient = osg::Vec4(0,0,0,1);
    osg::Vec4 diffuse = osg::Vec4(0.8,0.8,0.8,1);
    osg::Vec4 specular = osg::Vec4(1,1,1,1);
    osg::Light* light = viewer.getCamera()->getView()->getLight();
    if (light != NULL) {
        ambient = light->getAmbient();
        diffuse = light->getDiffuse();
        specular = light->getSpecular();
    }
    std::string content = "Multiple models :\n";
    for (std::map<std::string, osg::Node*>::iterator it=models.begin(); it!=models.end(); ++it) {
        content += "model :" + it->first + "\n";
        osg::Node* model = it->second;
        osg::ref_ptr<osg::PositionAttitudeTransform> tf = new osg::PositionAttitudeTransform();
        transforms.push_back(tf);
        tf->addChild(model);
        osg::ref_ptr<CullCallback> ccb = new CullCallback();
        tf->setCullCallback(ccb);
        cullCallbacks.push_back(ccb);
    }
    content += "configuration: \n" + config.getAsString();
    createInfo(folderName, content);

    cullCallbacks[0]->setEnabled(false);

    osg::ref_ptr<osg::Group> newRoot = new osg::Group();
    newRoot->addChild(bg_cam.get());
    for (int i = 0; i < transforms.size(); i++) {
        newRoot->addChild(transforms[i].get());
    }
    viewer.setSceneData(newRoot);

    int fileNameWidth = 0;
    for (int i = 0; i < translations.size(); i++) {
        Translation tr = translations[i];
        fileNameWidth += tr.count;
    }
    fileNameWidth = getWidth10(fileNameWidth);
    int imgIndex = 1;
    std::string labels = "file,px,py,pz,ax,ay,az,s\n";
    for (int i = 0; i < translations.size(); i++) {
        Translation tr = translations[i];
        for (int j = 0; j < tr.count; j++) {
            //random planes hiding
            int channels = randint(0, 16);
            int groupCount = 0;
            for (int k = 0; k < cullCallbacks.size(); k++) {
                bool enabled = channels & 1 << k;
                cullCallbacks[k]->setEnabled(enabled);
                if (!enabled) {
                    groupCount++;
                }
            }

            osg::Vec3d position = getPosition(tr, j);
            std::string fileShortName = intToString(imgIndex, fileNameWidth);
            std::string fileName = folderName + "/" + fileShortName + o.extension;
            setTranslation(transforms, tr, j, labels, fileShortName, groupCount);

            int bgPos = imgIndex % bgImages.size();
            osg::Image* bgImage = bgImages[bgPos];
            generateImage(bgImage, fileName, textureRect, viewer);

            //set no light
            if (light != NULL) {
                light->setAmbient(osg::Vec4(0,0,0,1));
                light->setDiffuse(osg::Vec4(0,0,0,1));
                light->setSpecular(osg::Vec4(0,0,0,1));
            }
            std::string maskFileName = o.maskFolder + "/bg_" + fileShortName + "_mask"+ o.extension;
            generateImage(maskBgImage, maskFileName, textureRect, viewer);
            for (int m = 0; m < transforms.size(); m++) {
                for (int k = 0; k < cullCallbacks.size(); k++) {
                    cullCallbacks[k]->setEnabled(true);
                }
                cullCallbacks[m]->setEnabled(channels & 1 << m);
                std::ostringstream ss;
                ss << m;
                maskFileName = o.maskFolder + "/" + ss.str() + "_" + fileShortName + "_mask"+ o.extension;
                generateImage(maskBgImage, maskFileName, textureRect, viewer);
            }
            //restore to initial light
            if (light != NULL) {
                light->setAmbient(ambient);
                light->setDiffuse(diffuse);
                light->setSpecular(specular);
            }

            imgIndex++;
        }
    }
    return 0;
}

/**
    Generates a set of images and correspondent masks according to defined configuration.
    @return 0 on success, or 1 if error occur
*/
int ImgGenerator::generateImages() {
    std::vector<osg::Image*> bgImages = config.loadBackground();
    if (bgImages.size() == 0) {
        osg::notify(osg::NOTICE)<<"No background images found"<<std::endl;
        return 1;
    }
    std::map<std::string, osg::Node*> models = config.loadModels();
    if (models.size() == 0) {
        osg::notify(osg::NOTICE)<<"No models found"<<std::endl;
        return 1;
    }
    osg::Image* maskBgImage;
    if (mode == 3) {
        maskBgImage = config.loadMaskBackground();
    }
    //check output folder
    int bgWidth = bgImages[0]->s();
    int bgHeight = bgImages[0]->t();

    osg::ref_ptr<osg::Camera> bg_cam = createBackgroundCamera();
    osg::ref_ptr<osg::TextureRectangle> textureRect =
        createBackgroundTexture(bg_cam.get(), bgWidth, bgHeight);
    //multisamles antialiasing
    osg::DisplaySettings::instance()->setNumMultiSamples(config.getOutput().numMultiSamples);
    osgViewer::Viewer viewer;
    osg::ref_ptr<osg::Group> root = new osg::Group();
    osg::ref_ptr<osg::PositionAttitudeTransform> oldModelTf = new osg::PositionAttitudeTransform();
    root->addChild(oldModelTf);
    root->addChild(bg_cam.get());

    viewer.setSceneData(root);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);

    viewer.getCamera()->setClearMask(GL_DEPTH_BUFFER_BIT);
    int xoffset = 0;
    int yoffset = 0;
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = xoffset + 0;
    traits->y = yoffset + 0;
    traits->width = bgWidth;
    traits->height = bgHeight;
    std::cout << "traits: " << traits->x << ", "<<traits->y <<", "<<traits->width<<", "<<traits->height<< std::endl;
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->sharedContext = 0;
    traits->samples = config.getOutput().numMultiSamples;

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer.getCamera()->setGraphicsContext(gc.get());
    viewer.getCamera()->setViewport(new osg::Viewport(0,0, traits->width, traits->height));
    GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
    viewer.getCamera()->setDrawBuffer(buffer);
    viewer.getCamera()->setReadBuffer(buffer);

    if (mode == 1 || mode == 3) {
        osg::ref_ptr<SaveImageCallback> saveImageCallback = new SaveImageCallback(config.getOutput().width);
        viewer.getCamera()->setFinalDrawCallback(saveImageCallback.get());
    }

    viewer.realize();

    Output o = config.getOutput();
    std::cout  << "num_objects="<<o.numObjects<<std::endl;
    Bounds b = o.objShifts;
    std::cout << "positions: " <<b.x_from<<" "<<b.x_to<<" "<<b.y_from<<", "<<b.y_to<<" "<<b.z_from<<" "<<b.z_to<<std::endl;
    int imgIdx = 1;
    std::vector<Translation> translations = config.getTranslations();
    std::vector<osg::ref_ptr<osg::PositionAttitudeTransform> > transforms;

    int folderNameWidth = getFolderWidth10(models.size());
    osg::Vec4 ambient = osg::Vec4(0,0,0,1);
    osg::Vec4 diffuse = osg::Vec4(0.8,0.8,0.8,1);
    osg::Vec4 specular = osg::Vec4(1,1,1,1);
    osg::Light* light = viewer.getCamera()->getView()->getLight();
    if (light != NULL) {
        ambient = light->getAmbient();
        diffuse = light->getDiffuse();
        specular = light->getSpecular();
    }
    if (mode == 3) {
        if (!dirExists(o.maskFolder)) {
            makeDir(o.maskFolder);
        }
    }
    int k = 0;
    for (std::map<std::string, osg::Node*>::iterator it=models.begin(); it!=models.end(); ++it) {
        Output o = config.getOutput();
        std::string folderName = o.folder + "/" + intToString(k, folderNameWidth);
        if (mode == 1 || mode == 3) {
            if (!dirExists(folderName)) {
                makeDir(folderName);
            }
            std::string content = "model :" + it->first + "\n";
            content += "configuration: \n" + config.getAsString();
            createInfo(folderName, content);
        }
        osg::Node* newModel = it->second;
        osg::ref_ptr<osg::Group> newRoot = new osg::Group();
        transforms.clear();
        for (int i = 0; i < config.getOutput().numObjects; i++) {
            osg::ref_ptr<osg::PositionAttitudeTransform> tf = new osg::PositionAttitudeTransform();
            transforms.push_back(tf);
            tf->addChild(newModel);
            newRoot->addChild(tf);
        }
        newRoot->addChild(bg_cam.get());
        viewer.setSceneData(newRoot);
        int modelImgIndex = 1;
        int fileNameWidth = 0;
        for (int i = 0; i < translations.size(); i++) {
            Translation tr = translations[i];
            fileNameWidth += tr.count;
        }
        fileNameWidth = getWidth10(fileNameWidth);
        std::string labels = "file,px,py,pz,ax,ay,az,s\n";
        for (int i = 0; i < translations.size(); i++) {
            Translation tr = translations[i];
            for (int j = 0; j < tr.count; j++) {
                osg::Vec3d position = getPosition(tr, j);
                std::string fileShortName = intToString(modelImgIndex, fileNameWidth);
                std::string fileName = folderName + "/" + fileShortName + o.extension;
                setTranslation(transforms, tr, j, labels, fileShortName, transforms.size());

                int bgPos = imgIdx % bgImages.size();
                osg::Image* bgImage = bgImages[bgPos];
                generateImage(bgImage, fileName, textureRect, viewer);
                if (mode == 3) {
                    //set no light
                    if (light != NULL) {
                        light->setAmbient(osg::Vec4(0,0,0,1));
                        light->setDiffuse(osg::Vec4(0,0,0,1));
                        light->setSpecular(osg::Vec4(0,0,0,1));
                    }
                    std::string maskFileName;
                    if (models.size() > 1) {
                        std::ostringstream ss;
                        ss << k;
                        maskFileName = o.maskFolder + "/" + ss.str() + "_" + fileShortName + "_mask"+ o.extension;
                    }
                    else {
                        maskFileName = o.maskFolder + "/" + fileShortName + "_mask"+ o.extension;
                    }

                    generateImage(maskBgImage, maskFileName, textureRect, viewer);
                    //restore to initial light
                    if (light != NULL) {
                        light->setAmbient(ambient);
                        light->setDiffuse(diffuse);
                        light->setSpecular(specular);
                    }
                }
                if (viewer.done()) {
                    return 0;
                }
                modelImgIndex++;
                imgIdx++;
            }
        }
        if (mode == 1 || mode == 3) {
            createLabels(folderName, labels);
        }
        k++;
    }
    return 0;
}

/**
    Sets position, attitude and scale of transformation using specified Translation object.
    Also appends current transformation values to 'labels' string;
    @param modelTf the transformation.
    @param tr the Translation.
    @param j the int specified current counter of calculations.
    @param labels the list of generated image names with correspondent transformation values.
    @param fileShortName the file name of generated image.
*/
void ImgGenerator::setTranslation(osg::ref_ptr<osg::PositionAttitudeTransform> modelTf, Translation tr, int j,
            std::string &labels, std::string fileShortName) {
    osg::Vec3d position = getPosition(tr, j);
    modelTf->setPosition( position );
    osg::Vec3d angles = getRotation(tr, j);
    osg::Quat rot(angles.x(), osg::X_AXIS, angles.y(), osg::Y_AXIS, angles.z(), osg::Z_AXIS);
    modelTf->setAttitude(rot);
    osg::Vec3d vScale = getScale(tr, j);
    modelTf->setScale(vScale);
    labels.append(fileShortName).append(",").
                    append(NumberToString(position.x())).append(",").
                    append(NumberToString(position.y())).append(",").
                    append(NumberToString(position.z())).append(",").
                    append(NumberToString(angles.x())).append(",").
                    append(NumberToString(angles.y())).append(",").
                    append(NumberToString(angles.z())).append(",").
                    append(NumberToString(vScale.x())).append("\n");
}

/**
    Sets position, attitude and scale of transformations using specified Translation object.
    Also appends current transformation values to 'labels' string;
    @param transforms the list of transformations.
    @param tr the Translation.
    @param j the int specified current counter of calculations.
    @param labels the list of generated image names with correspondent transformation values.
    @param fileShortName the file name of generated image.
    @param groupCount count of transformations in group, in range 1..4.
*/
void ImgGenerator::setTranslation(std::vector<osg::ref_ptr<osg::PositionAttitudeTransform> > transforms, Translation tr, int j,
            std::string &labels, std::string fileShortName, int groupCount) {
    osg::Vec3d position = getPosition(tr, j);
    setGroupShift(transforms, position, groupCount);
    osg::Vec3d angles = getRotation(tr, j);
    osg::Quat rot(angles.x(), osg::X_AXIS, angles.y(), osg::Y_AXIS, angles.z(), osg::Z_AXIS);
    osg::Vec3d vScale = getScale(tr, j);
    for (int i = 0; i < transforms.size(); i++) {
        transforms[i]->setAttitude(rot);
        transforms[i]->setScale(vScale);
    }
    labels.append(fileShortName).append(",").
                    append(NumberToString(position.x())).append(",").
                    append(NumberToString(position.y())).append(",").
                    append(NumberToString(position.z())).append(",").
                    append(NumberToString(angles.x())).append(",").
                    append(NumberToString(angles.y())).append(",").
                    append(NumberToString(angles.z())).append(",").
                    append(NumberToString(vScale.x())).append("\n");
}

/**
    Generates image, then SaveImageCallback save it to file. Image augmentation can be applied.
    @param image the background image.
    @param fileName file name to save image.
    @param textureRect the background texture.
    @param viewer osg Viewer.
*/
void ImgGenerator::generateImage(osg::Image* image, std::string fileName,
    osg::ref_ptr<osg::TextureRectangle> &textureRect, osgViewer::Viewer &viewer) {
    osg::ref_ptr<osg::Image> clone = osg::clone( image, osg::CopyOp::DEEP_COPY_ALL );
    if (config.bgAugmentation()) {
        double hFlip = (double)rand() / RAND_MAX;
        double vFlip = (double)rand() / RAND_MAX;
        double hSize = (double)rand() / RAND_MAX + 1.0;
        double vSize = (double)rand() / RAND_MAX + 1.0;
        double s = (double)(clone->s());
        double t = (double)clone->t();

        clone->scaleImage((int)(clone->s() * hSize), (int)(clone->t() * vSize), clone->r());

        double sZero = 0.0;
        double tZero = 0.0;
        osg::Image* cropped = cropImage(clone, 0.0, 0.0, (double)clone->s(), (double)clone->t(), sZero, tZero, s, t );
        if (hFlip > 0.5) {
            cropped->flipHorizontal();
        }
        if (vFlip > 0.5) {
            cropped->flipVertical();
        }
        cropped->dirty();
        textureRect->setImage(cropped);
    }
    else {
        clone->dirty();
        textureRect->setImage(clone.get());
    }
    osg::ref_ptr<SaveImageCallback> saveImageCallback =
        dynamic_cast<SaveImageCallback*>(viewer.getCamera()->getFinalDrawCallback());
    if(saveImageCallback.get()) {
        saveImageCallback->setFinished(false);
        saveImageCallback->setFileName(fileName);
    }
    viewer.frame();
    if (mode == 0) {
        usleep(300000);
    }
    else {
        if(saveImageCallback.get()) {
            int maxDelay = 0;
            while (!saveImageCallback.get()->isFinished() && maxDelay < 100) {
                usleep(30000);
                maxDelay++;
            }
        }
    }
}

/**
    Generates vector of new object position according to specified translation
    @param tr the Translation.
    @param j the int specified current counter of calculations.
    @return 3d vector, components is a new object position
*/
osg::Vec3d ImgGenerator::getPosition(Translation tr, int j) {
    double xShift, yShift, zShift;
    if (tr.random) {
        xShift = getRand(tr.position.x_from, tr.position.x_to);
        yShift = getRand(tr.position.y_from, tr.position.y_to);
        zShift = getRand(tr.position.z_from, tr.position.z_to);
    }
    else {
        xShift = tr.position.x_from + tr.position.x_delta * j;
        yShift = tr.position.y_from + tr.position.y_delta * j;
        zShift = tr.position.z_from + tr.position.z_delta * j;
    }
    osg::Vec3d position(xShift, yShift, zShift);
    return position;
}

/**
    Generates vector of new object rotation according to specified translation
    @param tr the Translation.
    @param j the int specified current counter of calculations.
    @return 3d vector, components is a new object angles
*/
osg::Vec3d ImgGenerator::getRotation(Translation tr, int j) {
    double xAngle, yAngle, zAngle;
    if (tr.random) {
        xAngle = getRand(tr.angle.x_from, tr.angle.x_to);
        yAngle = getRand(tr.angle.y_from, tr.angle.y_to);
        zAngle = getRand(tr.angle.z_from, tr.angle.z_to);
    }
    else {
        xAngle = tr.angle.x_from + tr.angle.x_delta * j;
        yAngle = tr.angle.y_from + tr.angle.y_delta * j;
        zAngle = tr.angle.z_from + tr.angle.z_delta * j;
    }
    osg::Vec3d angles(xAngle, yAngle, zAngle);
    return angles;
}

/**
    Generates vector of new object scale according to specified translation
    @param tr the Translation.
    @param j the int specified current counter of calculations.
    @return 3d vector, all components is equals - a new scale
*/
osg::Vec3d ImgGenerator::getScale(Translation tr, int j) {
    double scale;
    if (tr.random) {
        scale = getRand(tr.scale_from, tr.scale_to);
    }
    else {
        scale = tr.scale_from + tr.scale_delta * j;
    }
    osg::Vec3d vScale(scale, scale, scale);
    return vScale;
}

/**
    Returns a pseudo-random double between specified from and to values.
    @param from the double minimal range value.
    @param to the double maximal range value.
    @return a pseudo-random double between specified from and to values.
*/
double ImgGenerator::getRand(double from, double to) {
    double f = (double)rand() / RAND_MAX;
    return from + f * (to - from);
}

/**
    Returns new image cropped using specified parameters.
    src_ values defines source image window, dst_ values - destination image window
    @param image initial image.
    @param src_minx the double minimal source x position.
    @param src_miny the double minimal source y position.
    @param src_maxx the double maximal source x position.
    @param src_maxy the double maximal source y position.
    @param dst_minx the double minimal destination x position.
    @param dst_miny the double minimal destination y position.
    @param dst_maxx the double maximal destination x position.
    @param dst_maxy the double maximal destination y position.
    @return a new cropped image.
*/
osg::Image* ImgGenerator::cropImage(const osg::Image* image,
                      double src_minx, double src_miny, double src_maxx, double src_maxy,
                      double &dst_minx, double &dst_miny, double &dst_maxx, double &dst_maxy)
{
    if ( image == 0L )
        return 0L;

    //Compute the desired cropping rectangle
    int windowX        = osg::clampBetween( (int)floor( (dst_minx - src_minx) / (src_maxx - src_minx) * (double)image->s()), 0, image->s()-1);
    int windowY        = osg::clampBetween( (int)floor( (dst_miny - src_miny) / (src_maxy - src_miny) * (double)image->t()), 0, image->t()-1);
    int windowWidth    = osg::clampBetween( (int)ceil(  (dst_maxx - src_minx) / (src_maxx - src_minx) * (double)image->s()) - windowX, 0, image->s());
    int windowHeight   = osg::clampBetween( (int)ceil(  (dst_maxy - src_miny) / (src_maxy - src_miny) * (double)image->t()) - windowY, 0, image->t());

    if (windowX + windowWidth > image->s())
    {
        windowWidth = image->s() - windowX;
    }

    if (windowY + windowHeight > image->t())
    {
        windowHeight = image->t() - windowY;
    }

    if ((windowWidth * windowHeight) == 0)
    {
        return NULL;
    }

    //Compute the actual bounds of the area we are computing
    double res_s = (src_maxx - src_minx) / (double)image->s();
    double res_t = (src_maxy - src_miny) / (double)image->t();

    dst_minx = src_minx + (double)windowX * res_s;
    dst_miny = src_miny + (double)windowY * res_t;
    dst_maxx = dst_minx + (double)windowWidth * res_s;
    dst_maxy = dst_miny + (double)windowHeight * res_t;

    //Allocate the cropped image
    osg::Image* cropped = new osg::Image;
    cropped->allocateImage(windowWidth, windowHeight, image->r(), image->getPixelFormat(), image->getDataType());
    cropped->setInternalTextureFormat( image->getInternalTextureFormat() );

    for (int layer=0; layer<image->r(); ++layer)
    {
        for (int src_row = windowY, dst_row=0; dst_row < windowHeight; src_row++, dst_row++)
        {
            if (src_row > image->t()-1) {
                osg::notify(osg::NOTICE) << "HeightBroke" << std::endl;
            }
            const void* src_data = image->data(windowX, src_row, layer);
            void* dst_data = cropped->data(0, dst_row, layer);
            memcpy( dst_data, src_data, cropped->getRowSizeInBytes());
        }
    }
    return cropped;
}

/**
    Calculates and sets random shift for list of transformation.
    This transformations used to generate some objects from one object in result image, shifted one from the other randomly.
    @param transforms a vector of PositionAttitudeTransform objects.
    @param position the center of group.
    @param groupCount count of objects in group, in range 1..4.
*/
void ImgGenerator::setGroupShift(std::vector<osg::ref_ptr<osg::PositionAttitudeTransform> > transforms, osg::Vec3d position, int groupCount) {
    int signX = getRand(0., 2.) > 1. ? 1 : -1;
    int signY = getRand(0., 2.) > 1. ? 1 : -1;
    int signZ = getRand(0., 2.) > 1. ? 1 : -1;
    if (groupCount == 1) {
        transforms[0]->setPosition( position );
    }
    else if (groupCount == 2) {
        setGroupShift(transforms[0], transforms[1], position, signX, signY, signZ);
    }
    else if (groupCount == 3) {
        setGroupShift(transforms[0], transforms[1], position, signX, signY, signZ);
        transforms[2]->setPosition(position);
    }
    else if (groupCount == 4) {
        setGroupShift(transforms[0], transforms[1], position, signX, signY, signZ);
        setGroupShift(transforms[2], transforms[3], position, signX, signY, -signZ);
    }
}

/**
    Calculates and sets specified shift for pair of transformation.
    @param tr the first PositionAttitudeTransform object.
    @param tr the second PositionAttitudeTransform object.
    @param position the center of group.
    @param signX sign of shift for x coordinate.
    @param signY sign of shift for y coordinate.
    @param signZ sign of shift for z coordinate.
*/
void ImgGenerator::setGroupShift(osg::ref_ptr<osg::PositionAttitudeTransform> tr,
        osg::ref_ptr<osg::PositionAttitudeTransform> tr2,
        osg::Vec3d position, int signX, int signY, int signZ) {
    float sX = config.getOutput().objShifts.x_from;
    float sY = config.getOutput().objShifts.y_from;
    float sZ = config.getOutput().objShifts.z_from;
    float tX = config.getOutput().objShifts.x_to;
    float tY = config.getOutput().objShifts.y_to;
    float tZ = config.getOutput().objShifts.z_to;
    float x1 = getRand(sX, tX) * signX;
    float x2 = -x1 + position.x();
    x1 += position.x();
    float y1 = getRand(sY, tY) * signY;
    float y2 = -y1 + position.y();
    y1 += position.y();
    float z1 = getRand(sZ, tZ) * signZ;
    float z2 = -z1 + position.z();
    z1 += position.z();
    tr->setPosition(osg::Vec3d(x1, y1, z1));
    tr2->setPosition(osg::Vec3d(x2, y2, z2));
}

