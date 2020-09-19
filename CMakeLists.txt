cmake_minimum_required (VERSION 2.8)
project(Video)

#指定源文件
SET(CMAKE_CXX_COMPILER "g++")

option(BUILD_SHARED_LIBS "Build using sharedlibraries" ON)


#添加sub目录
#add_subdirectory(test)

aux_source_directory(src DIR_SRCS)

SET(CMAKE_COMMON_LD_FLAGS "")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wall -g")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")

add_definitions("-fPIC")
add_definitions("-std=c++11")

SET(THIRDLIB_PATH "/home/dgyx/work/thirdlib/install/")
include_directories("include")
include_directories("${THIRDLIB_PATH}/glog/include")
include_directories("${THIRDLIB_PATH}/gflags/include")
include_directories("${THIRDLIB_PATH}/ffmpeg/include")
include_directories("${THIRDLIB_PATH}/opencv/include")

link_directories("${THIRDLIB_PATH}/glog/lib")
link_directories("${THIRDLIB_PATH}/gflags/lib")
link_directories("${THIRDLIB_PATH}/ffmpeg/lib")
link_directories("${THIRDLIB_PATH}/opencv/lib")

add_library(video ${DIR_SRCS})
target_link_libraries(video 
		glog 
		gflags
		swscale
		avcodec
		avformat
		avutil
		glog
		pthread)
