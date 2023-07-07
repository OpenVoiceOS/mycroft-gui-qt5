PROJECT(mycroftguiapp)

include(ECMSetupVersion)

PROJECT(mycroftguiapp VERSION "1.0.1")
    SET(mycroft_gui_app_SRC
        main.cpp
        appsettings.cpp
        speechintent.cpp
    )

ecm_setup_version(PROJECT VERSION_HEADER version.h)

set(mycroft_gui_app_EXTRA_LIBS
        Qt5::Widgets
        KF5::DBusAddons
    )

qt5_add_resources(mycroft_gui_app_SRC
    qml.qrc
)

add_executable(ovos-gui-app ${mycroft_gui_app_SRC})
target_link_libraries(ovos-gui-app Qt5::Core Qt5::Quick Qt5::WebView ${mycroft_gui_app_EXTRA_LIBS})


install(TARGETS ovos-gui-app ${INSTALL_TARGETS_DEFAULT_ARGS})
install( PROGRAMS ai.ovos.gui-app.desktop DESTINATION  ${KDE_INSTALL_APPDIR} )



