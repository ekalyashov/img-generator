#ifndef SAVEIMAGECALLBACK_H
#define SAVEIMAGECALLBACK_H

#include <osg/Camera>


class SaveImageCallback : public osg::Camera::DrawCallback
{
    public:
        SaveImageCallback(int _outputWidth);
        virtual ~SaveImageCallback();

        void setFileName(const std::string& aFileName) { fileName = aFileName; }

        virtual void operator () (const osg::Camera& camera) const;
        bool isFinished() { return finished; }
        void setFinished(bool _finished) { finished = _finished;}
    protected:
        std::string _prefix;
        std::string _fileId;
        std::string fileName;
        //output image width and height
        int outputWidth;
    private:
        bool mutable finished;
};

#endif // SAVEIMAGECALLBACK_H
