cmake_minimum_required(VERSION 3.19)
project(ReferenceTelemetryTest LANGUAGES CXX)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets SerialPort Charts)
qt_standard_project_setup()
get_filename_component(WORKSPACE "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
set(HOST "${WORKSPACE}/03-HostApp/01_SerialConsole")
qt_add_executable(reference_telemetry_test
    test_telemetry.cpp
    "${HOST}/mainwindow.cpp" "${HOST}/mainwindow.h" "${HOST}/mainwindow.ui"
    "${HOST}/protocolworker.cpp" "${HOST}/protocolworker.h")
target_include_directories(reference_telemetry_test PRIVATE "${HOST}")
target_link_libraries(reference_telemetry_test PRIVATE Qt::Core Qt::Widgets Qt::SerialPort Qt::Charts)
