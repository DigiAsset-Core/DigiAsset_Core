cmake_minimum_required(VERSION 3.21)
project(digiasset_core-qt LANGUAGES CXX)

# --- C++ standard ---
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Enable automatic Qt MOC, UIC, and RCC ---
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Extra module paths, if needed ---
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# --- Find external dependencies ---
# JSONCPP/JSONRPCCPP are resolved further down with the same cascade the daemon uses
# (src/CMakeLists.txt).  The legacy Find modules only look in /usr, /usr/local and
# win32-deps, so on Windows under vcpkg they find nothing - which is why the GUI could
# not be built natively there.
find_package(CURL REQUIRED)

# --- Find Qt libraries ---
# Works with Qt5 or Qt6 - whichever is installed
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Charts Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Charts Network)

# --- Source files ---
set(HEADER_FILES
        PlotManager.h
        RPCLoader.h
        AssetIconProvider.h
        tabs/SyncTab.h
        tabs/BalancesTab.h
        tabs/SendAssetTab.h
        tabs/CreateAssetTab.h
        tabs/HistoryTab.h
        tabs/ManageAssetTab.h
        ../src/Config.h
        ../src/DigiByteCore.h
        ../src/DigiAssetConstants.h
        ../src/utils.h
        ../src/Log.h
        )

set(SOURCE_FILES
        PlotManager.cpp
        RPCLoader.cpp
        AssetIconProvider.cpp
        tabs/SyncTab.cpp
        tabs/BalancesTab.cpp
        tabs/SendAssetTab.cpp
        tabs/CreateAssetTab.cpp
        tabs/HistoryTab.cpp
        tabs/ManageAssetTab.cpp
        ../src/Config.cpp
        ../src/DigiByteCore.cpp
        ../src/utils.cpp
        #DigiByteCore.cpp says which wallet it picked and complains when core has more than one
        #loaded and the config does not say which to use, so it needs Log.cpp, same as the cli.
        #Log.cpp has no dependencies of its own
        ../src/Log.cpp
        )

# --- Create the executable ---
# On macOS this has to be a .app bundle: macdeployqt only works on bundles, and without it the
# released binary points at the build machine's Qt frameworks and will not start anywhere else.
# Linux and Windows are unaffected - they stay plain executables and are bundled by
# linuxdeploy and windeployqt respectively.
if(APPLE)
    set(DIGIASSET_QT_BUNDLE MACOSX_BUNDLE)
endif()

add_executable(digiasset_core-qt ${DIGIASSET_QT_BUNDLE}
        main.cpp
        ${SOURCE_FILES}
        ${HEADER_FILES}
        )

if(APPLE)
    set_target_properties(digiasset_core-qt PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME "DigiAsset Core"
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.digiassetcore.qt"
            MACOSX_BUNDLE_BUNDLE_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}"
            )
endif()

# --- Add application icon AFTER target creation ---
set(APP_ICON_RESOURCE ../bin/images/app_icon.rc)
target_sources(digiasset_core-qt PRIVATE ${APP_ICON_RESOURCE})

# --- Link system libraries (curl, ssl, crypto, pthread) ---
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(digiasset_core-qt PRIVATE CURL::libcurl)
target_link_libraries(digiasset_core-qt PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)

# --- Link JSON libraries ---
# JsonCpp: try vcpkg/cmake config first, then pkg-config (Linux/macOS), then custom Find module
find_package(jsoncpp CONFIG QUIET)
if(TARGET jsoncpp_lib)
    target_link_libraries(digiasset_core-qt PRIVATE jsoncpp_lib)
elseif(TARGET jsoncpp_object)
    target_link_libraries(digiasset_core-qt PRIVATE jsoncpp_object)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(JSONCPP jsoncpp)
    endif()
    if(JSONCPP_FOUND)
        target_include_directories(digiasset_core-qt PRIVATE ${JSONCPP_INCLUDE_DIRS})
        target_link_libraries(digiasset_core-qt PRIVATE ${JSONCPP_LIBRARIES})
    else()
        find_package(JSONCPP REQUIRED)  # custom cmake/FindJSONCPP.cmake
        target_include_directories(digiasset_core-qt PRIVATE ${JSONCPP_INCLUDE_DIRS})
        target_link_libraries(digiasset_core-qt PRIVATE ${JSONCPP_LIBRARIES})
    endif()
endif()

# libjson-rpc-cpp: try vcpkg/cmake config first, then plain library names
find_package(libjson-rpc-cpp CONFIG QUIET)
if(TARGET libjson-rpc-cpp::jsonrpccpp-common)
    target_link_libraries(digiasset_core-qt PRIVATE
            libjson-rpc-cpp::jsonrpccpp-common libjson-rpc-cpp::jsonrpccpp-client)
else()
    find_package(JSONRPCCPP REQUIRED)  # custom cmake/FindJSONRPCCPP.cmake
    target_include_directories(digiasset_core-qt PRIVATE ${JSONRPCCPP_INCLUDE_DIRS})
    target_link_libraries(digiasset_core-qt PRIVATE jsonrpccpp-common jsonrpccpp-client)
endif()

# Windows: the RPC client pulls in Winsock
if(WIN32)
    target_link_libraries(digiasset_core-qt PRIVATE ws2_32)
endif()

# --- Link SQLite (if needed) ---
# target_link_libraries(digiasset_core-qt PUBLIC sqlite3)
# If you have a static library for sqlite3, set it up similarly.

# --- Link Qt libraries (Qt5 or Qt6, whichever find_package found above) ---
target_link_libraries(digiasset_core-qt
        PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Charts
        Qt${QT_VERSION_MAJOR}::Network
        )



# --- Installation rule ---
install(TARGETS digiasset_core-qt DESTINATION bin)