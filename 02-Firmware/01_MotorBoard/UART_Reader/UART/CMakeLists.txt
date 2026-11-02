cmake_minimum_required(VERSION 3.19)
project(UART LANGUAGES CXX)

# 设置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 为MSVC添加必要的编译选项
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 COMPONENTS SerialPort REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core)


qt_standard_project_setup()

qt_add_executable(UART
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    protocolhandler.h protocolhandler.cpp
    MotorUartProtocol.h MotorUartProtocol.cpp
    SerialWorker.h SerialWorker.cpp
)

target_link_libraries(UART
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::SerialPort
        -Wl,--allow-multiple-definition
)


include(GNUInstallDirs)

install(TARGETS UART
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET UART
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
