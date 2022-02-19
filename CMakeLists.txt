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
	
	target_link_libraries(${PROJECT_NAME} steam_api)
	
else()
    set(CMAKE_CXX_FLAGS "-Wall")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-Os -fno-exceptions -w -Wfatal-errors")
endif()



