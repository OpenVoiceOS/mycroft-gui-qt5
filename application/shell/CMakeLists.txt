# OVOSPlugin QML module — provides Configuration and PlacesModel singletons
set(openvoiceshell_SRCS openvoiceshellplugin.cpp configuration.cpp placesmodel.cpp)

add_library(openvoiceshellplugin SHARED ${openvoiceshell_SRCS})

target_link_libraries(openvoiceshellplugin
    Qt5::Core
    Qt5::Quick
    Qt5::Qml
    Qt5::DBus
    KF5::GuiAddons
    KF5::ConfigWidgets
)

install(TARGETS openvoiceshellplugin DESTINATION ${KDE_INSTALL_QMLDIR}/OVOSPlugin)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/OVOSPlugin)
install(FILES plugins.qmltypes DESTINATION ${KDE_INSTALL_QMLDIR}/OVOSPlugin)
