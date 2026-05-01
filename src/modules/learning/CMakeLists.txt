qt_add_library(algemate_learning STATIC
    LearningPage.h
    LearningPage.cpp
)

target_link_libraries(algemate_learning
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_include_directories(algemate_learning
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)
