cmake_minimum_required(VERSION 3.19)
project(DesktopSReferenceValidation LANGUAGES CXX)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets SerialPort Charts)
qt_standard_project_setup()
set(DESKTOP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/build/source")
add_subdirectory("${DESKTOP_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/application")
qt_add_executable(desktop_reference_test
    test_desktop_reference.cpp
    "${DESKTOP_SOURCE}/mainwindow.h" "${DESKTOP_SOURCE}/mainwindow.ui"
    "${DESKTOP_SOURCE}/protocolworker.cpp" "${DESKTOP_SOURCE}/protocolworker.h")
target_include_directories(desktop_reference_test PRIVATE "${DESKTOP_SOURCE}")
set_property(TARGET desktop_reference_test PROPERTY AUTOUIC_SEARCH_PATHS "${DESKTOP_SOURCE}")
target_link_libraries(desktop_reference_test PRIVATE Qt::Core Qt::Widgets Qt::SerialPort Qt::Charts)
enable_testing()
add_test(NAME desktop_reference COMMAND desktop_reference_test)
set_tests_properties(desktop_reference PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
