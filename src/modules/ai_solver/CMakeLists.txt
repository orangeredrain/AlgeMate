qt_add_library(algemate_ai_solver STATIC
    AiSolverPage.h
    AiSolverPage.cpp
    DeepSeekClient.h
    DeepSeekClient.cpp
    MessageBubble.h
    MessageBubble.cpp
)

target_link_libraries(algemate_ai_solver
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
)

target_include_directories(algemate_ai_solver
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)
