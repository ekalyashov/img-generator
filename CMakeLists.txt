cmake_minimum_required(VERSION 2.8)

project(generator)

SET(TARGET_SRC src/generator.cpp src/SaveImageCallback.cpp src/jsoncpp.cpp src/Configurator.cpp src/ImgGenerator.cpp)

SET(OSG_PATH ~/work/OpenSceneGraph)

include_directories(${OSG_PATH}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${OSG_PATH}/lib64)
ADD_EXECUTABLE(generator ${TARGET_SRC})
TARGET_LINK_LIBRARIES(generator OpenThreads osg osgUtil osgText osgDB osgGA osgViewer)
configure_file(config.json config.json COPYONLY)
