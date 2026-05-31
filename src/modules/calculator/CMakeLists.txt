qt_add_library(algemate_calculator STATIC
    # 外壳
    CalculatorPage.h
    CalculatorPage.cpp
    # 交互式 · Notebook UI
    interactive/InteractivePage.h
    interactive/InteractivePage.cpp
    interactive/HelpDialog.h
    interactive/HelpDialog.cpp
    interactive/LatexTextBrowser.h
    interactive/LatexTextBrowser.cpp
    # 交互式 · 表达式引擎
    interactive/expr/RenderSettings.h
    interactive/expr/RenderSettings.cpp
    interactive/expr/Value.h
    interactive/expr/Value.cpp
    interactive/expr/Lexer.h
    interactive/expr/Lexer.cpp
    interactive/expr/Parser.h
    interactive/expr/Parser.cpp
    interactive/expr/Evaluator.h
    interactive/expr/Evaluator.cpp
    # 可视化
    visualize/VisualizePage.h
    visualize/VisualizePage.cpp
    visualize/QuadricWidget.h
    visualize/QuadricWidget.cpp
    # 算法演示
    demo/DemoCommon.h
    demo/EigenPage.h
    demo/EigenPage.cpp
    demo/GSOPage.h
    demo/GSOPage.cpp
    demo/MaxIndepPage.h
    demo/MaxIndepPage.cpp
    demo/DemoPage.h
    demo/DemoPage.cpp
    demo/HomoLinearSystemPage.h
    demo/HomoLinearSystemPage.cpp
    demo/InversePage.h
    demo/InversePage.cpp
    demo/JordanFormPage.h
    demo/JordanFormPage.cpp
    demo/SymDiagPage.h
    demo/SymDiagPage.cpp
    demo/NonhomoLinearSystemPage.h
    demo/NonhomoLinearSystemPage.cpp
    demo/PolyGCDPage.h
    demo/PolyGCDPage.cpp
    demo/QuadFormPage.h
    demo/SymReducePage.h
    demo/SymReducePage.cpp
    demo/QuadFormPage.cpp

)

target_link_libraries(algemate_calculator
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::OpenGLWidgets
        algemate_math
    PRIVATE
        JKQTMathTextLib
)

target_include_directories(algemate_calculator
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
