cmake_minimum_required(VERSION 3.19)
project(DirectFilePushClient LANGUAGES CXX)

# Find Qt6 (will use system host version or mingw-w64 if toolchain is active)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

qt_add_executable(DirectFilePushClient
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    message.h message.cpp message.ui
    filemessage.h filemessage.cpp filemessage.ui
)

target_link_libraries(DirectFilePushClient
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
)

include(GNUInstallDirs)

install(TARGETS DirectFilePushClient
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET DirectFilePushClient
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
