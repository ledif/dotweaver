add_executable(dotweaver
    main.cpp
    mainwindow.cpp
    mainwindow.h
    chezmoiservice.cpp
    chezmoiservice.h
    dotfilemanager.cpp
    dotfilemanager.h
    configeditor.cpp
    configeditor.h
    filetab.cpp
    filetab.h
    logger.cpp
    logger.h
    logviewer.cpp
    logviewer.h
    statusbar.cpp
    statusbar.h
    ../resources.qrc
)

# Enable Qt MOC for header files
set_target_properties(dotweaver PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)

target_link_libraries(dotweaver
    Qt6::Core
    Qt6::Widgets
    Qt6::DBus
    KF6::CoreAddons
    KF6::I18n
    KF6::ConfigWidgets
    KF6::XmlGui
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::IconThemes
    KF6::WidgetsAddons
    KF6::TextWidgets
    KF6::Notifications
)

# Include directories for generated files
target_include_directories(dotweaver PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
)

# Install the executable
install(TARGETS dotweaver DESTINATION ${KDE_INSTALL_BINDIR})

# Install desktop file and icons
install(FILES ../io.github.ledif.dotweaver.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES ../icons/dotweaver.png DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/256x256/apps/)
