PROJECT(mycroftguiapp)
include(ECMSetupVersion)

PROJECT(mycroftguiapp VERSION "1.0.1")
    SET(mycroft_gui_app_SRC
        main.cpp
        appsettings.cpp)

ecm_setup_version(PROJECT VERSION_HEADER version.h)


set(mycroft_gui_app_EXTRA_LIBS
    Qt${QT_MAJOR_VERSION}::Widgets
    KF6::DBusAddons
)
# endif()

qt_add_resources(mycroft_gui_app_SRC
    qml.qrc
)

add_executable(mycroft-gui-app ${mycroft_gui_app_SRC})
target_link_libraries(mycroft-gui-app Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Quick Qt${QT_MAJOR_VERSION}::WebView ${mycroft_gui_app_EXTRA_LIBS})


install(TARGETS mycroft-gui-app ${INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS ai.mycroft.gui-app.desktop DESTINATION  ${KDE_INSTALL_APPDIR})

