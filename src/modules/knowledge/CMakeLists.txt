qt_add_library(algemate_knowledge STATIC
    KnowledgePage.h
    KnowledgePage.cpp
)

target_link_libraries(algemate_knowledge
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

target_include_directories(algemate_knowledge
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)
