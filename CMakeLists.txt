cmake_minimum_required(VERSION 3.19)
project(DoctorMangerSystem LANGUAGES CXX)
set(CMAKE_AUTOUIC ON)  #自动处理 UI 文件。
set(CMAKE_AUTOMOC ON)  #自动处理元对象编译器（MOC）
set(CMAKE_AUTORCC ON)  #自动处理资源文件
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Charts)
qt_standard_project_setup()

qt_add_executable(DoctorMangerSystem
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    res.qrc
    databasemangager.h databasemangager.cpp
    doctorinfowidget.h doctorinfowidget.cpp doctorinfowidget.ui
    tableDelegates.h
    schedulewidget.h schedulewidget.cpp schedulewidget.ui
    financialwidget.h financialwidget.cpp financialwidget.ui

)

target_link_libraries(DoctorMangerSystem
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Sql
        Qt6::Charts
)

install(TARGETS DoctorMangerSystem
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET DoctorMangerSystem
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

#添加避免找不到sql库文件目录
INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}
)

