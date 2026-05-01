qt_add_library(algemate_settings STATIC
    SettingsPage.h
    SettingsPage.cpp
)

target_link_libraries(algemate_settings
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_include_directories(algemate_settings
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)
