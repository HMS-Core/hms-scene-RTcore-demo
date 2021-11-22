cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)

set(NAME triangle)

set(PROJ_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
set(SRC_DIR ../../../examples/${NAME})
set(BASE_DIR ../../../base)
set(EXTERNAL_DIR ../../../3rdparty)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DVK_USE_PLATFORM_ANDROID_KHR")

file(GLOB EXAMPLE_SRC "${SRC_DIR}/*.cpp")

add_library(native-lib SHARED ${EXAMPLE_SRC})

add_library(native-app-glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_subdirectory(../base ${CMAKE_SOURCE_DIR}/../base)
#add_subdirectory(../../../data ${CMAKE_SOURCE_DIR}/../../../data)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

include_directories(${BASE_DIR})
include_directories(${EXTERNAL_DIR})
include_directories(${EXTERNAL_DIR}/glm)
include_directories(${EXTERNAL_DIR}/imgui)
include_directories(${EXTERNAL_DIR}/tinygltf)
include_directories(${PROJ_ROOT}/include)
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)


target_link_libraries(
    native-lib
    native-app-glue
    libbase
    android
    log
    z
)
