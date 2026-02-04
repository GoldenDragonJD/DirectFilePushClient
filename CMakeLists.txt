cmake_minimum_required(VERSION 3.19)
project(DirectFilePushClient LANGUAGES CXX)

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
    addipwindow.h addipwindow.cpp addipwindow.ui
    notificationmanager.h notificationmanager.cpp
    encryptionwindow.h encryptionwindow.cpp encryptionwindow.ui
    SimpleStreamCipher.h
    hmac.h


)

target_link_libraries(DirectFilePushClient
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
)

include(GNUInstallDirs)

# Standard install for Linux/Flatpak
install(TARGETS DirectFilePushClient
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# ONLY run deployment for Windows/macOS/Local bundles.
# FLATPAK_ID is automatically set by flatpak-builder.
if(NOT DEFINED ENV{FLATPAK_ID})
    qt_generate_deploy_app_script(
        TARGET DirectFilePushClient
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
