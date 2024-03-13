cmake_minimum_required(VERSION 3.16)

# get number of commits in current branch
execute_process(
  COMMAND git rev-list HEAD --first-parent --count
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_COUNT
  OUTPUT_STRIP_TRAILING_WHITESPACE)

project(
  marsh-manager
  VERSION 0.1.0.${GIT_COMMIT_COUNT}
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17) # minimum required by Qt6
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Network QuickControls2 Gui
                                         Widgets)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appmarsh-manager src/main.cpp)
set_target_properties(appmarsh-manager PROPERTIES OUTPUT_NAME marsh-mgr)

qt_add_qml_module(
  appmarsh-manager
  URI
  marsh-manager
  VERSION
  1.0
  QML_FILES
  src/Main.qml
  SOURCES
  src/router.h
  src/router.cpp
  SOURCES
  src/applicationdata.h
  src/applicationdata.cpp
  SOURCES
  src/clientnode.h
  src/clientnode.cpp
  SOURCES
  src/message.h
  src/message.cpp
  QML_FILES
  src/HelpMenu.qml
  SOURCES
  src/networkdisplay.h
  src/networkdisplay.cpp
  QML_FILES
  src/NetworkView.qml
  SOURCES
  src/logger.h
  src/logger.cpp)

qt_add_resources(appmarsh-manager "app_text" PREFIX "/" FILES LICENSE.txt)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1. If
# you are developing for iOS or macOS you should consider setting an explicit,
# fixed bundle identifier manually though.
set_target_properties(
  appmarsh-manager
  PROPERTIES # MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appmarsh-manager
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE)

target_include_directories(appmarsh-manager PRIVATE modules)

# get the latest abbreviated commit hash
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

target_compile_definitions(
  appmarsh-manager
  PRIVATE MAVLINK_USE_MESSAGE_INFO=1
  # prevent MSVC from redefining same constants that Qt provides
  PRIVATE _MATH_DEFINES_DEFINED=1
  PRIVATE APP_GIT_COMMIT_COUNT="${GIT_COMMIT_COUNT}"
  PRIVATE APP_GIT_HASH="${GIT_HASH}"
  PRIVATE APP_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
  # add git commit number to Debug build version
  PRIVATE
    APP_VERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}$<$<CONFIG:Debug>:+${PROJECT_VERSION_TWEAK}>"
)

target_link_libraries(
  appmarsh-manager
  PRIVATE Qt6::Quick
  PRIVATE Qt6::Network
  PRIVATE Qt6::QuickControls2
  PRIVATE Qt6::Gui
  PRIVATE Qt6::Widgets)

include(GNUInstallDirs)
install(
  TARGETS appmarsh-manager
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
