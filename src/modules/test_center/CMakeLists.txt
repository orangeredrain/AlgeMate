qt_add_library(algemate_test_center STATIC
    TestCenterPage.h
    TestCenterPage.cpp
)

target_link_libraries(algemate_test_center
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        algemate_latex
)

target_include_directories(algemate_test_center
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)
