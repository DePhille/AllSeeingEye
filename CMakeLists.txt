cmake_minimum_required (VERSION 2.8.11)
project (Ase)

##set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/resources/cmake/")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /usr/local/lib/cmake/)

## use CCache if available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    MESSAGE(STATUS "Will use CCACHE")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
else ()
    MESSAGE(STATUS "CCACHE NOT AVAILABLE")
endif(CCACHE_FOUND)

## Compiler flags
if (CMAKE_BUILD_TYPE MATCHES RELEASE)
    MESSAGE(STATUS "Building in RELEASE mode")
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "-O3 -g0 -Wall -march=armv6zk -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -std=c++0x")
    endif()
else()
    MESSAGE(STATUS "Building in DEBUG mode")
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "-O2 -g3 -Wall -march=armv6zk -mcpu=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp -std=c++0x")
    endif()
endif()

find_package(PiEye REQUIRED)
find_package(raspicam REQUIRED)
find_package(OpenCV)

add_executable(Ase
	src/main
	src/CameraTuner
)

target_link_libraries (Ase ${raspicam_CV_LIBS} PiEye)