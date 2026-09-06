cmake_minimum_required(VERSION 3.10)
project(rfcomm_connector CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_library(BLUETOOTH_LIBRARY bluetooth REQUIRED)

add_executable(rfcomm_connector
    src/main.cpp
    src/BluetoothConnector.cpp
)

target_include_directories(rfcomm_connector PRIVATE lib)
target_compile_options(rfcomm_connector PRIVATE -fpermissive)
target_link_libraries(rfcomm_connector PRIVATE ${BLUETOOTH_LIBRARY})
