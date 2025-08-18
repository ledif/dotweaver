cmake_minimum_required(VERSION 3.20)

project(dotweaver
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A dotfile management application using chezmoi backend"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required Qt6 and KF6 components
find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMQtDeclareLoggingCategory)

find_package(Qt6 6.4.0 REQUIRED COMPONENTS
    Core
    Widgets
    DBus
)

find_package(KF6 REQUIRED COMPONENTS
    CoreAddons
    I18n
    ConfigWidgets
    XmlGui
    KIO
    IconThemes
    WidgetsAddons
    TextWidgets
    TextEditor
    Notifications
)

# Set up version
ecm_setup_version(PROJECT
    VARIABLE_PREFIX DOTWEAVER
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/dotweaver_version.h"
)

# Add subdirectories
add_subdirectory(src)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Feature summary
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
