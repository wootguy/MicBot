cmake_minimum_required(VERSION 3.5)
project(steam_voice)

set(SOURCE_FILES 
    src/main.cpp
	src/webm.cpp
	src/webm_util.h src/webm_util.cpp
	src/opus_util.h src/opus_util.cpp
	src/mp4_util.h src/mp4_util.cpp
)

include_directories(src)
include_directories(include)
link_directories(lib)

if(MSVC)
    # compile using the static runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	
	# Disable C++ exceptions
	#string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(LINK_LIBS ${CMAKE_CURRENT_LIST_DIR}/lib/opus.lib 
				  ${CMAKE_CURRENT_LIST_DIR}/lib/libcurl-x64.lib
				  ${CMAKE_CURRENT_LIST_DIR}/lib/webm_parser.lib)

else()
    set(CMAKE_CXX_FLAGS "-Wall -m32")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -fno-exceptions -w -Wfatal-errors")

    set(LINK_LIBS ${CMAKE_CURRENT_LIST_DIR}/lib/libsteam_api.so)
    set(LINK_OPTIONS -R .) # load libraries from the same directory as the program
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (MSVC)
	set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS} ${OPTIONS})


