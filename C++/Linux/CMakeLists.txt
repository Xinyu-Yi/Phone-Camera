cmake_minimum_required( VERSION 3.4 )
project( PhoneCamera )
set( CMAKE_CXX_STANDARD 14 )
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread" )

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_library( PhoneCamera PhoneCamera.cpp )
add_executable( record_video record_video.cpp)
target_link_libraries( record_video PhoneCamera ${OpenCV_LIBS} )

# build python wrapper
add_subdirectory( ../../extern/pybind11 pybind11 )
pybind11_add_module( phonecam pybind.cpp mat_wrapper.cpp PhoneCamera.cpp )
target_link_libraries( phonecam PRIVATE ${OpenCV_LIBS} )
