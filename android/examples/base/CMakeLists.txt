cmake_minimum_required(VERSION 3.4.1 FATAL_ERROR)

find_package(Vulkan REQUIRED)
if (${VULKAN_FOUND})
	set(VULKAN_DIR ${Vulkan_INCLUDE_DIR})
	set(vulkan-lib ${Vulkan_LIBRARY})
	message(STATUS ${VULKAN_DIR})
	message(STATUS ${Vulkan_LIBRARY})
else ()
	message(FATAL_ERROR "Vulkan Not Found!")
endif ()

set(PROJ_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DVK_USE_PLATFORM_ANDROID_KHR")
file(GLOB BASE_SRC "${PROJ_ROOT}/base/*.cpp" "${PROJ_ROOT}/3rdparty/imgui/*.cpp" "${PROJ_ROOT}/3rdparty/SaschaWillemsVulkan/*.cpp" "${PROJ_ROOT}/3rdparty/SaschaWillemsVulkan/*.hpp")

# librtcore.so导入
set(RAYSHOP_LIB rtcore)

include_directories(${PROJ_ROOT}/include)

ADD_LIBRARY(
		${RAYSHOP_LIB}
		SHARED
		IMPORTED)

SET_TARGET_PROPERTIES(${RAYSHOP_LIB}
		PROPERTIES IMPORTED_LOCATION
		${PROJ_ROOT}/libs/${ANDROID_ABI}/librtcore.so)

add_library(libbase SHARED ${BASE_SRC} ${RAYSHOP_SRC_FILES})

include_directories(${BASE_DIR})
include_directories(${PROJ_ROOT}/3rdparty)
include_directories(${PROJ_ROOT}/3rdparty/glm)
include_directories(${PROJ_ROOT}/3rdparty/gli)
include_directories(${PROJ_ROOT}/3rdparty/imgui)
include_directories(${PROJ_ROOT}/3rdparty/tinygltf)
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)

set(KTX_DIR ${PROJ_ROOT}/3rdparty/ktx)
set(KTX_SOURCES
	${KTX_DIR}/lib/texture.c
	${KTX_DIR}/lib/hashlist.c
	${KTX_DIR}/lib/checkheader.c
	${KTX_DIR}/lib/swap.c
	${KTX_DIR}/lib/memstream.c
	${KTX_DIR}/lib/filestream.c
)
set(KTX_INCLUDE
	${KTX_DIR}/include
	${KTX_DIR}/lib
	${KTX_DIR}/other_include
)

add_library(libktx ${KTX_SOURCES})
target_include_directories(libktx PUBLIC ${KTX_INCLUDE})
set_property(TARGET libktx PROPERTY FOLDER "3rdparty")

target_link_libraries(
	libbase
	android
	log
	z
	libktx
	vulkan
	${RAYSHOP_LIB}
)
