cmake_minimum_required(VERSION 2.8)
project( VIBE )
find_package( OpenCV REQUIRED )
# add_library( CBDriver SHARED CBDriver.cpp )
add_executable( main-opencv main-opencv.cpp )
add_library( Vibe vibe-background-sequential.c )
target_link_libraries( main-opencv Vibe ${OpenCV_LIBS} )
