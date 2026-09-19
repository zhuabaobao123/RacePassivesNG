cmake_minimum_required(VERSION 3.21)

project(
	RacePassives
	LANGUAGES CXX
	VERSION 2.1
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Runtime targets. Flat (SE + AE) by default; -DENABLE_SKYRIM_VR=ON for the VR variant.
set(ENABLE_SKYRIM_SE ON CACHE BOOL " " FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL " " FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL " " FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Disable CommonLib tests in consumer" FORCE)

# CommonLibSSE-NG source tree (alandtse ng line).
set(COMMONLIB_SSE_FOLDER "E:/CommonLibSSE-NG-alandtse" CACHE PATH "CommonLibSSE-NG source directory")

add_subdirectory(${COMMONLIB_SSE_FOLDER} CommonLibSSE EXCLUDE_FROM_ALL)
get_target_property(COMMONLIB_SRC_DIR CommonLibSSE SOURCE_DIR)
include(${COMMONLIB_SRC_DIR}/cmake/CommonLibSSE.cmake)

add_commonlibsse_plugin(${PROJECT_NAME}
	AUTHOR "RacePassives"
	SOURCES
		src/main.cpp
		src/UI.cpp
		src/Localization.cpp
		src/Config.cpp
		src/SyncScale.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/PCH.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /Zc:__cplusplus)

	# Keep debug symbols in Release for Crash Logger stack resolution.
	target_compile_options(${PROJECT_NAME} PRIVATE /Zi)
	target_link_options(${PROJECT_NAME} PRIVATE /DEBUG /OPT:REF /OPT:ICF)
endif()
