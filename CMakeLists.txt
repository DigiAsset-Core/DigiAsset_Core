cmake_minimum_required(VERSION 3.24)
project(digiasset_core)

option(BUILD_CLI "Build CLI" ON)
option(BUILD_WEB "Build Web Server" ON)
option(BUILD_QT "Build QT" ON)
option(BUILD_TEST "Build Tests" OFF)


# Set compiler settings
set(CMAKE_CXX_STANDARD 11)
if(MSVC)
    set(CMAKE_CXX_FLAGS "/W4 /EHsc")
    add_definitions(-D_WIN32_WINNT=0x0601)   # Windows 7+
    add_definitions(-DNOMINMAX)              # Prevent Windows.h min/max macros colliding with std::min/max
    add_definitions(-DWIN32_LEAN_AND_MEAN)   # Trim unused Windows headers
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
endif()

# Set version number
SET(MAJOR_VERSION 1)
SET(MINOR_VERSION 0)
SET(PATCH_VERSION 0)
SET(SO_VERSION    1)

# Set output directory for binaries and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)  # For executables
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)  # For shared libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)  # For static libraries


# Add source directory
include_directories(src)
ADD_SUBDIRECTORY(src)

#create cli
if(BUILD_CLI)
    message(STATUS "\nConfiguring CLI:")
    include_directories(cli)
    ADD_SUBDIRECTORY(cli)
endif()

#create qt
if(BUILD_QT)
    message(STATUS "\nConfiguring QT:")
    include_directories(qt)
    ADD_SUBDIRECTORY(qt)
endif()

#create web
if(BUILD_WEB)
    message(STATUS "\nConfiguring Web:")
    include_directories(web)
    ADD_SUBDIRECTORY(web)
endif()

#build test script
if(BUILD_TEST)
    message(STATUS "\nConfiguring Tests:")
    set(GOOGLETEST_VERSION 1.11.0)
    ADD_SUBDIRECTORY(tests)
endif()