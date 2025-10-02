# Tell CMake we are cross-compiling for Windows
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 1)

# Compiler setup
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Where the MinGW-w64 Qt is installed (adjust this if different on your system)
# On Arch-based distros, mingw-w64-qt6 installs here:
set(CMAKE_PREFIX_PATH "/usr/x86_64-w64-mingw32/qt6/lib/cmake")

# Optional install dir
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
