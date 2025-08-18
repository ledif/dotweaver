add_executable(kchezmoi
    main.cpp
    mainwindow.cpp
    chezmoiservice.cpp
    dotfilemanager.cpp
    configeditor.cpp
)

target_link_libraries(kchezmoi
    Qt6::Core
    Qt6::Widgets
    Qt6::DBus
    KF6::CoreAddons
    KF6::I18n
    KF6::Config
    KF6::ConfigWidgets
    KF6::XmlGui
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::IconThemes
    KF6::WidgetsAddons
    KF6::TextWidgets
    KF6::Notifications
)

# Install the executable
install(TARGETS kchezmoi DESTINATION ${KDE_INSTALL_BINDIR})

# Install desktop file and icons
install(FILES org.kde.kchezmoi.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ../icons/kchezmoi.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps/)

# Install UI files
install(FILES ui/kchezmoi.xml DESTINATION ${KDE_INSTALL_DATADIR}/kchezmoi)
