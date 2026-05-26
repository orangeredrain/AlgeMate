qt_add_library(algemate_learning STATIC
    ClickableCard.h
    LearningPage.h
    LearningPage.cpp
    KnowledgePage.h
    KnowledgePage.cpp
    PracticePage.h
    PracticePage.cpp
    ExamPage.h
    ExamPage.cpp
    WrongBookPage.h
    WrongBookPage.cpp
    LearningCenterPage.h
    LearningCenterPage.cpp
    ApiClient.h
    ApiClient.cpp
    QuestionType.h
)

target_link_libraries(algemate_learning
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        algemate_latex
)

target_include_directories(algemate_learning
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)
