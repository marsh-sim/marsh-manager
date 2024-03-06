cmake_minimum_required(VERSION 3.16)

project(marsh-manager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appmarsh-manager
    src/main.cpp
)
set_target_properties(appmarsh-manager PROPERTIES
    OUTPUT_NAME marsh-mgr
)

qt_add_qml_module(appmarsh-manager
    URI marsh-manager
    VERSION 1.0
    QML_FILES
    src/Main.qml
    SOURCES src/router.h src/router.cpp
    SOURCES src/applicationdata.h src/applicationdata.cpp
    SOURCES src/clientnode.h src/clientnode.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appmarsh-manager PROPERTIES
    # MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appmarsh-manager
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

find_package(Qt6 REQUIRED COMPONENTS Network)

target_include_directories(appmarsh-manager
    PRIVATE modules
)

target_compile_definitions(appmarsh-manager
    PRIVATE MAVLINK_USE_MESSAGE_INFO=1
)

target_link_libraries(appmarsh-manager
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Network
)

include(GNUInstallDirs)
install(TARGETS appmarsh-manager
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
