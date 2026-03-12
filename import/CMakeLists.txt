project(mycroftimport)

# Compile-time default path for SYSTEM: template resolution.
# Shells (ovos-shell, etc.) can override at runtime via OVOS_SYSTEM_TEMPLATES env var.
if(NOT MYCROFT_SYSTEM_TEMPLATES_DIR)
    set(MYCROFT_SYSTEM_TEMPLATES_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/mycroft-gui/system-templates")
endif()

set(mycroftimport_SRCS
    mycroftplugin.cpp
    mycroftcontroller.cpp
    activeskillsmodel.cpp
    delegatesmodel.cpp
    abstractskillview.cpp
    abstractdelegate.cpp
    sessiondatamap.cpp
    sessiondatamodel.cpp
    globalsettings.cpp
    filereader.cpp
    mediaservice.cpp
    thirdparty/fftcalc.cpp
    thirdparty/fft.cpp
    )

configure_file(controllerconfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/controllerconfig.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

qt5_add_resources(mycroftimport_SRCS mycroft.qrc)

add_library(mycroftplugin SHARED ${mycroftimport_SRCS} ${RESOURCES})

target_link_libraries(mycroftplugin
        PUBLIC
            Qt5::Core
            Qt5::Multimedia
            ${mycroftplugin_EXTRA_LIBS}
        PRIVATE
            Qt5::Qml
            Qt5::Quick
            Qt5::Network
            Qt5::WebSockets
    )

install(TARGETS mycroftplugin DESTINATION ${KDE_INSTALL_QMLDIR}/Mycroft)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/Mycroft)
install(DIRECTORY qml/ DESTINATION ${KDE_INSTALL_QMLDIR}/Mycroft)
install(FILES plugins.qmltypes DESTINATION ${KDE_INSTALL_QMLDIR}/Mycroft)

# Install default SYSTEM: template QML files.
# Shells that bundle their own templates should set OVOS_SYSTEM_TEMPLATES at
# launch time to point to their own directory; see docs/system-templates.md.
install(DIRECTORY system-templates/ DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/mycroft-gui/system-templates"
    FILES_MATCHING PATTERN "*.qml")
