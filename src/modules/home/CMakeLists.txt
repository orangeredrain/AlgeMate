qt_add_library(algemate_home STATIC
    HomePage.h
    HomePage.cpp
)

target_link_libraries(algemate_home
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_include_directories(algemate_home
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)
