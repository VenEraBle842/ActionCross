cmake_minimum_required(VERSION 3.14)
project(ActionCross CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        adtlinear
        GIT_REPOSITORY https://github.com/VenEraBle842/ADTLinearOrganization.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(adtlinear)

set(PGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PGE_HEADER "${PGE_DIR}/olcPixelGameEngine.h")
if(NOT EXISTS "${PGE_HEADER}")
    file(MAKE_DIRECTORY "${PGE_DIR}")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/OneLoneCoder/olcPixelGameEngine/master/olcPixelGameEngine.h"
            "${PGE_HEADER}"
            SHOW_PROGRESS
            STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download olcPixelGameEngine.h")
    endif()
endif()

set(SOURCES src/main.cpp)
add_executable(ActionCross ${SOURCES})

target_include_directories(ActionCross PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"            # olcPixelGameEngine.h
        "${CMAKE_CURRENT_SOURCE_DIR}/src"                # project headers
        "${adtlinear_SOURCE_DIR}/src"                    # ADTLinearOrganization headers
)

if(WIN32)
    target_link_libraries(ActionCross PRIVATE
            user32 gdi32 opengl32 gdiplus Shlwapi dwmapi
    )
endif()
