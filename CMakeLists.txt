cmake_minimum_required(VERSION 3.21)
project(AlgeMate LANGUAGES CXX VERSION 1.0.0)

# ---------- C++ 标准 ----------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 注: JKQtPlotter 依赖若干 POSIX 数学函数, 必须开启 GNU 扩展
set(CMAKE_CXX_EXTENSIONS ON)

# ---------- 默认 Release ----------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# ---------- Release 优化 ----------
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_OK OUTPUT IPO_MSG)
if(IPO_OK)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
endif()

if(MSVC)
    add_compile_options(/utf-8 /MP /permissive-)
    add_compile_options($<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/Oi> $<$<CONFIG:Release>:/GL> $<$<CONFIG:Release>:/DNDEBUG>)
    add_link_options($<$<CONFIG:Release>:/LTCG> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
else()
    add_compile_options($<$<CONFIG:Release>:-O3> $<$<CONFIG:Release>:-DNDEBUG>)
endif()

# ==========================================
# 关键：必须先找 Qt，CMake 才会认识 qt_add_executable！
# ==========================================
# ---------- Qt 6 ----------
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Gui Svg Xml OpenGL OpenGLWidgets Network)
qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---------- third_party: JKQTMathText ----------
set(JKQtPlotter_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(JKQtPlotter_BUILD_STATIC_LIBS ON  CACHE BOOL "" FORCE)
set(JKQtPlotter_BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(JKQtPlotter_BUILD_TOOLS       OFF CACHE BOOL "" FORCE)
set(JKQtPlotter_BUILD_DECORATE_LIBNAMES_WITH_BUILDTYPE OFF CACHE BOOL "" FORCE)
set(JKQtPlotter_BUILD_WITH_PRECOMPILED_HEADERS OFF CACHE BOOL "" FORCE)
set(JKQtPlotter_BUILD_ONLY_MATHTEXT ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/JKQtPlotter-master)

# ---------- 目录组织 ----------
set(ALGEMATE_SRC_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(ALGEMATE_RES_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(ALGEMATE_MODULES_DIR    ${ALGEMATE_SRC_DIR}/modules)

add_subdirectory(${ALGEMATE_SRC_DIR}/math)
add_subdirectory(${ALGEMATE_MODULES_DIR}/home)
add_subdirectory(${ALGEMATE_MODULES_DIR}/calculator)
add_subdirectory(${ALGEMATE_MODULES_DIR}/ai_solver)
add_subdirectory(${ALGEMATE_MODULES_DIR}/learning)
add_subdirectory(${ALGEMATE_MODULES_DIR}/settings)
add_subdirectory(${ALGEMATE_MODULES_DIR}/test_center)
add_subdirectory(${ALGEMATE_SRC_DIR}/latex)

# ---------- 核心壳目标 ----------
set(MACOS_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon.icns")
set_source_files_properties(${MACOS_APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

# 1. 定义源文件
set(ALGEMATE_SOURCES
    main.cpp
    # core
    src/core/ThemeManager.h
    src/core/ThemeManager.cpp
    src/core/AppPaths.h
    src/core/AppPaths.cpp
    src/core/UserProfile.h
    src/core/UserProfile.cpp
    # ui
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.ui
    src/ui/TitleBar.h
    src/ui/TitleBar.cpp
    src/ui/NavigationBar.h
    src/ui/NavigationBar.cpp
    src/ui/UserProfileDialog.h
    src/ui/UserProfileDialog.cpp
    resources/resources.qrc
)

# 2. 如果是 Windows 才添加 rc 资源
if(WIN32)
    list(APPEND ALGEMATE_SOURCES resources/icon.rc)
endif()

# 3. 添加执行文件
qt_add_executable(AlgeMate
    WIN32 MACOSX_BUNDLE
    ${MACOS_APP_ICON}
    ${ALGEMATE_SOURCES}
    src/ui/TomatoManager.cpp
    src/ui/TomatoManager.h
    src/ui/TomatoClockDialog.h
    src/ui/TomatoClockDialog.cpp
)

target_include_directories(AlgeMate PRIVATE ${ALGEMATE_SRC_DIR})

target_link_libraries(AlgeMate
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
        algemate_math
        algemate_home
        algemate_calculator
        algemate_ai_solver
        algemate_learning
        algemate_settings
        algemate_test_center
        algemate_latex
)

target_compile_definitions(AlgeMate PRIVATE
    ALGEMATE_VERSION_STRING="${PROJECT_VERSION}"
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)
set_target_properties(AlgeMate PROPERTIES
    MACOSX_BUNDLE_ICON_FILE "icon.icns"
)

# ---------- 安装 / 部署 ----------
include(GNUInstallDirs)
install(TARGETS AlgeMate
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET AlgeMate
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})