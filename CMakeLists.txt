cmake_minimum_required(VERSION 3.10)
project(iv VERSION 0.1.0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

# Set flags for Debug build
set(CMAKE_CXX_FLAGS_DEBUG " -fno-omit-frame-pointer -ggdb3 -O0 -Wall -Wextra")

# Set flags for Release build
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Sql)

add_definitions( -DMAGICKCORE_QUANTUM_DEPTH=16 )
add_definitions( -DMAGICKCORE_HDRI_ENABLE=0 )
find_package(ImageMagick COMPONENTS Magick++)


find_package(PkgConfig REQUIRED)
pkg_check_modules(MAGICKPP REQUIRED Magick++)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/ImageView.cpp
    src/MainWindow.cpp
    src/Panel.cpp
    src/GraphicsView.cpp
    src/ElidableLabel.hpp
    src/TabWidget.cpp
    src/Minimap.hpp
    src/Config.hpp
    src/PropertiesWidget.hpp
    src/OverlayRect.hpp
)

include_directories(${MAGICKPP_INCLUDE_DIRS})
link_directories(${MAGICKPP_LIBRARY_DIRS})
include_directories(${ImageMagick_INCLUDE_DIRS})
qt_add_resources(RESOURCES resources.qrc)

target_sources(${PROJECT_NAME} PRIVATE ${RESOURCES})

target_link_libraries(${PROJECT_NAME}
    Qt6::Widgets
    Qt6::Core
    ${ImageMagick_LIBRARIES}
    ${MAGICKPP_LIBRARIES}
)

find_library(libavif avif)
find_library(libexiv2 exiv2)

if(libavif)
    message(STATUS "*libavif* found. Compiling with AVIF image file support")
    target_link_libraries(${PROJECT_NAME} avif)
    target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_LIBAVIF=1)
else()
    message(STATUS "libavif not found; AVIF support will be disabled")
endif()

if(libexiv2)
    message(STATUS "*libexiv2* found. Compiling with EXIF metadata support")
    target_link_libraries(${PROJECT_NAME} exiv2)
    target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_LIBEXIV2=1)
else()
    message(STATUS "libexiv2 not found; EXIF metadata support will be disabled")
endif()


install(TARGETS ${PROJECT_NAME} DESTINATION bin)

install(FILES ${PROJECT_NAME}.desktop DESTINATION share/applications)

# Install icons to correct freedesktop-compliant locations
install(DIRECTORY resources/icons/hicolor
    DESTINATION share/icons
    FILES_MATCHING PATTERN "*.png")
