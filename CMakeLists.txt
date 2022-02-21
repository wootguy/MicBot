cmake_minimum_required(VERSION 3.5)
project(steam_voice)

set(SOURCE_FILES 
    src/main.cpp
)

include_directories(src)
include_directories(include)
link_directories(lib)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(MSVC)
    # compile using the static runtime
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /wd4244 /wd4018")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /wd4244 /wd4018")
	
	# Disable C++ exceptions
	#string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    set(LINK_LIBS steam_api)

else()
    set(CMAKE_CXX_FLAGS "-Wall -m32")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -fno-exceptions -w -Wfatal-errors")

    set(LINK_LIBS ${CMAKE_CURRENT_LIST_DIR}/lib/libsteam_api.so)
    set(LINK_OPTIONS -R .) # load libraries from the same directory as the program
endif()

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS} ${OPTIONS})


