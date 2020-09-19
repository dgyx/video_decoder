cmake_minimum_required (VERSION 2.8)
project(demo)

#指定源文件
SET(CMAKE_CXX_COMPILER "g++")

#添加sub目录
#add_subdirectory(test)

aux_source_directory(. DIR_SRCS)

SET(CMAKE_COMMON_LD_FLAGS "")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -Wall -g")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")

SET(RUNTIME_LIB_PATH "/home/dgyx/work/thirdlib/lib/")
add_definitions("-fPIC")
add_definitions("-std=c++11")
#add_definitions("-Wl,-rpath=${RUNTIME_LIB_PATH}")

SET(LINK_FLAG "-Wl,-rpath=.:${RUNTIME_LIB_PATH} -Wl,-rpath-link=.:${RUNTIME_LIB_PATH}")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${LINK_FLAG}")
set(CMAKE_SKIP_BUILD_RPATH TRUE)
#SET(CMAKE_INSTALL_RPATH ${RUNTIME_LIB_PATH})

SET(THIRDLIB_PATH "/home/dgyx/work/thirdlib/install/")
include_directories("../include")
include_directories("${THIRDLIB_PATH}/glog/include")
include_directories("${THIRDLIB_PATH}/gflags/include")
include_directories("${THIRDLIB_PATH}/ffmpeg/include")
include_directories("${THIRDLIB_PATH}/opencv/include")

link_directories("../lib")
link_directories("${THIRDLIB_PATH}/glog/lib")
link_directories("${THIRDLIB_PATH}/gflags/lib")
link_directories("${THIRDLIB_PATH}/ffmpeg/lib")
link_directories("${THIRDLIB_PATH}/opencv/lib")

add_executable(demo ${DIR_SRCS})
target_link_libraries(demo
		video 
		glog 
		gflags
		swscale
		avcodec
		avformat
		avutil
		glog
		pthread)
