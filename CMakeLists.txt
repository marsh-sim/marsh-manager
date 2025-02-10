cmake_minimum_required(VERSION 3.16)

project(
  marsh-manager
  VERSION 0.2.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17) # minimum required by Qt6
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Network QuickControls2 Gui
                                         Widgets Graphs)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appmarsh-manager src/main.cpp)
set_target_properties(appmarsh-manager PROPERTIES OUTPUT_NAME marsh-mgr)

# Get last commit hash without the need to reconfigure By Jonathan Hamberg,
# under MIT License. see:
# https://jonathanhamberg.com/post/cmake-embedding-git-hash/
# https://gitlab.com/jhamberg/cmake-examples
include(cmake/CheckGit.cmake)
checkgitsetup()

qt_add_qml_module(
  appmarsh-manager
  URI
  marsh-manager
  VERSION
  1.0
  QML_FILES
  src/Main.qml
  src/HelpMenu.qml
  src/NetworkView.qml
  src/ToolsMenu.qml
  SOURCES
  src/router.h
  src/router.cpp
  src/applicationdata.h
  src/applicationdata.cpp
  src/clientnode.h
  src/clientnode.cpp
  src/message.h
  src/message.cpp
  src/networkdisplay.h
  src/networkdisplay.cpp
  src/logger.h
  src/logger.cpp
  src/appearanceutils.h
  src/appearanceutils.cpp
  src/heartbeatservice.h
  src/heartbeatservice.cpp
  src/dialectinfo.h
  src/dialectinfo.cpp
  src/parameterservice.h
  src/parameterservice.cpp
  src/frequencyestimator.h
  src/frequencyestimator.cpp
  src/networkdisplayproxy.h
  src/networkdisplayproxy.cpp
  src/plotbackend.h
  src/plotbackend.cpp)

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

# change list separator from semicolon to comma, to put in compile definitions
string(REPLACE ";" "," MESSAGE_DEFINITION_FILES_COMMA
               "${MESSAGE_DEFINITION_FILES}")

target_compile_definitions(
  appmarsh-manager
  PRIVATE
    MAVLINK_USE_MESSAGE_INFO=1
    # prevent MSVC from redefining same constants that Qt provides
    _MATH_DEFINES_DEFINED=1
    APP_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    # add git commit number to Debug build version
    APP_VERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
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
  PRIVATE Qt6::Widgets
  PRIVATE Qt6::Graphs
  PRIVATE git_version)

include(GNUInstallDirs)
install(
  TARGETS appmarsh-manager
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # Triggered a lot in MAVLink headers
  target_compile_options(appmarsh-manager PRIVATE -Wno-address-of-packed-member)
endif()
