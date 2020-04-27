cmake_minimum_required(VERSION 2.8.3)


ADD_DEFINITIONS(
    -std=c++11 # Or -std=c++0x
    # Other flags
)

project(camera_publisher)

find_package(OpenCV 4.1 REQUIRED)

find_package(PkgConfig)
pkg_check_modules(RS realsense2)

include_directories(
  ${OpenCV_INCLUDES}
  ${RS_INCLUDE_DIRS}
)

file(GLOB_RECURSE SOURCE_FILES "src/*")

#add_executable(camera_publisher src/main.cpp)
add_executable(image_handler ${SOURCE_FILES})


target_link_libraries(image_handler
  ${OpenCV_LIBRARIES}
  ${RS_LIBRARIES}
  pthread
)
