qt_add_library(algemate_calculator STATIC
    CalculatorPage.h
    CalculatorPage.cpp
)

target_link_libraries(algemate_calculator
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_include_directories(algemate_calculator
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)
