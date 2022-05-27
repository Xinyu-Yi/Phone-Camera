cmake_minimum_required( VERSION 2.8 )
project( PhoneCamera )
set( CMAKE_CXX_STANDARD 14 )
SET( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -pthread )

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_library( PhoneCamera PhoneCamera.cpp )
add_executable( record_video record_video.cpp)
target_link_libraries( record_video PhoneCamera ${OpenCV_LIBS} )


