cmake_minimum_required(VERSION 3.10)
project(rfcomm_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

add_executable(rfcomm_server
    src/main.cpp
    src/BluetoothServer.cpp
)

target_include_directories(rfcomm_server PRIVATE lib ${DBUS_INCLUDE_DIRS})
target_compile_options(rfcomm_server PRIVATE ${DBUS_CFLAGS_OTHER} -fpermissive)
target_link_libraries(rfcomm_server PRIVATE ${DBUS_LIBRARIES})
