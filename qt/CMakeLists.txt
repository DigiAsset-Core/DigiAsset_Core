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
find_package(JSONCPP REQUIRED)
find_package(JSONRPCCPP REQUIRED)
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
        ../src/DigiDollar.h
        ../src/Blob.h
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
        ../src/DigiDollar.cpp
        ../src/Blob.cpp
        ../src/utils.cpp
        #DigiByteCore.cpp says which wallet it picked and complains when core has more than one
        #loaded and the config does not say which to use, so it needs Log.cpp, same as the cli.
        #Log.cpp has no dependencies of its own
        ../src/Log.cpp
        )

# --- Create the executable ---
add_executable(digiasset_core-qt
        main.cpp
        ${SOURCE_FILES}
        ${HEADER_FILES}
        )

# --- Add application icon AFTER target creation ---
set(APP_ICON_RESOURCE ../bin/images/app_icon.rc)
target_sources(digiasset_core-qt PRIVATE ${APP_ICON_RESOURCE})

# --- Link system libraries (curl, ssl, crypto, pthread) ---
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(digiasset_core-qt PRIVATE CURL::libcurl)
target_link_libraries(digiasset_core-qt PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)

# --- Link JSON libraries ---
target_include_directories(digiasset_core-qt PRIVATE ${JSONCPP_INCLUDE_DIRS})
if(JSONCPP_LIBRARIES)
    target_link_libraries(digiasset_core-qt PRIVATE ${JSONCPP_LIBRARIES})
else()
    target_link_libraries(digiasset_core-qt PRIVATE jsoncpp)
endif()
target_link_libraries(digiasset_core-qt PRIVATE jsonrpccpp-common)
target_link_libraries(digiasset_core-qt PRIVATE jsonrpccpp-client)

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