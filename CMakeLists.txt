cmake_minimum_required(VERSION 3.19)
project(Game_Remote_Server LANGUAGES CXX)



find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(Game_Remote_Server
    WIN32 MACOSX_BUNDLE

    gui/main.cpp
    gui/mainwindow.cpp
    gui/mainwindow.h
    gui/mainwindow.ui

    core/src/utils.cpp
    core/src/controller_engine.cpp
    core/src/discovery_service.cpp
    core/src/udp_receiver.cpp
    core/src/virtual_gamepad.cpp

    core/include/utils.h
    core/include/controller_engine.h
    core/include/discovery_protocol.h
    core/include/discovery_service.h
    core/include/gamepad_mapping.h
    core/include/protocol.h
    core/include/udp_receiver.h
    core/include/virtual_gamepad.h
)

target_link_libraries(Game_Remote_Server
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS Game_Remote_Server
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Game_Remote_Server
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
