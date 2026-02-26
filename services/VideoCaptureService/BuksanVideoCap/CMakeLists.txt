cmake_minimum_required(VERSION 3.16)
project(BuksanVideoCap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED core videoio)
find_package(yaml-cpp REQUIRED)

set(SRC
    src/main.cpp
    src/ConfigLoader.cpp
    src/StorageManager.cpp
    src/Analytics.cpp
    src/Recorder.cpp
    src/CameraSession.cpp
    src/CameraManager.cpp
)

add_executable(BuksanVideoCap ${SRC})

target_include_directories(BuksanVideoCap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(BuksanVideoCap PRIVATE
    ${OpenCV_LIBS}
    yaml-cpp
)

install(TARGETS BuksanVideoCap RUNTIME DESTINATION bin)
