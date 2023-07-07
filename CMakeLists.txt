# Set minimum CMake version (required for CMake 3.0 or later)
cmake_minimum_required(VERSION 3.16.0)

set(QT_MAJOR_VERSION "6")
set(QT_MIN_VERSION "6.4.0")
set(KF_MIN_VERSION "5.240.0")

IF(POLICY CMP0048)
    CMAKE_POLICY(SET CMP0048 NEW)
ENDIF(POLICY CMP0048)

PROJECT(mycroft-gui)
PROJECT(mycroft-gui VERSION "1.0.1")

# Use Extra CMake Modules (ECM) for common functionality.
# See http://api.kde.org/ecm/manual/ecm.7.html
# and http://api.kde.org/ecm/manual/ecm-kde-modules.7.html
IF(POLICY CMP0063)
    CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)

# find_package(ECM 5.240.0 REQUIRED NO_MODULE)  <- from ovos-shell qt6 branch
find_package(ECM 5.240.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMInstallIcons)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Werror")

#configure_file(config-mycroft.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-mycroft.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Quick
    Core
    Qml
    Network
    WebSockets
    WebView
    Multimedia
)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS WebEngineQuick)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS I18n)


find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Kirigami2 DBusAddons)
# find_package(KF5Plasma ${KF5_MIN_VERSION} REQUIRED)
# find_package(KF5KIO ${KF5_MIN_VERSION} REQUIRED)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060F00)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING -DQT_USE_QSTRINGBUILDER -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_ASCII)

add_subdirectory(application)
add_subdirectory(icons)
add_subdirectory(import)

