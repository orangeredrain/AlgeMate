qt_add_library(algemate_latex STATIC
    LatexRenderer.h
    LatexRenderer.cpp
    LatexTextBrowser.h
    LatexTextBrowser.cpp
    LatexInlineHandler.h
    LatexInlineHandler.cpp
)

target_link_libraries(algemate_latex
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
    PRIVATE
        JKQTMathTextLib
)

target_include_directories(algemate_latex
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)
