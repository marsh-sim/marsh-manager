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
  src/logger.cpp
  SOURCES
  src/appearanceutils.h
  src/appearanceutils.cpp
  SOURCES
  src/heartbeatservice.h
  src/heartbeatservice.cpp
  SOURCES
  src/dialectinfo.h
  src/dialectinfo.cpp
  SOURCES
  src/parameterservice.h
  src/parameterservice.cpp)

qt_add_resources(appmarsh-manager "app_text" PREFIX "/" FILES LICENSE.txt)

# files included in all.xml
set(MESSAGE_DEFINITION_FILES
    all.xml
    ardupilotmega.xml
    ASLUAV.xml
    common.xml
    development.xml
    icarous.xml
    minimal.xml
    python_array_test.xml
    standard.xml
    test.xml
    ualberta.xml
    uAvionix.xml
    storm32.xml
    AVSSUAS.xml
    cubepilot.xml
    csAirLink.xml
    marsh.xml)

set(MESSAGE_DEFINITION_PATHS ${MESSAGE_DEFINITION_FILES})
set(MESSAGE_DEFINITION_BASE "include/mavlink/message_definitions")
list(TRANSFORM MESSAGE_DEFINITION_PATHS PREPEND "${MESSAGE_DEFINITION_BASE}/")
qt_add_resources(
  appmarsh-manager
  "message_definitions"
  PREFIX
  "/msgdef"
  BASE
  ${MESSAGE_DEFINITION_BASE}
  FILES
  ${MESSAGE_DEFINITION_PATHS})

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

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/modules/mavlink/pymavlink/setup.py)
  message(
    FATAL_ERROR
      "Some dependencies missing, run: git submodule update --init --recursive")
endif()

if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/include/mavlink/all/mavlink.h)
  message(
    FATAL_ERROR "MAVLink library missing, run: python scripts/update_mavlink.py"
  )
endif()

target_include_directories(appmarsh-manager PRIVATE include)

# get the latest abbreviated commit hash
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# change list separator from semicolon to comma, to put in compile definitions
string(REPLACE ";" "," MESSAGE_DEFINITION_FILES_COMMA
               "${MESSAGE_DEFINITION_FILES}")

target_compile_definitions(
  appmarsh-manager
  PRIVATE
    MAVLINK_USE_MESSAGE_INFO=1
    # prevent MSVC from redefining same constants that Qt provides
    _MATH_DEFINES_DEFINED=1
    APP_GIT_COMMIT_COUNT="${GIT_COMMIT_COUNT}"
    APP_GIT_HASH="${GIT_HASH}"
    APP_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    # add git commit number to Debug build version
    APP_VERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}$<$<CONFIG:Debug>:+${PROJECT_VERSION_TWEAK}>"
    # suppress the "QML debugging is enabled..." warning
    QT_QML_DEBUG_NO_WARNING=1
    MESSAGE_DEFINITION_FILES="${MESSAGE_DEFINITION_FILES_COMMA}")

# get_target_property(COMPDEFS appmarsh-manager COMPILE_DEFINITIONS)
# message("${COMPDEFS}")

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
