cmake_minimum_required(VERSION 3.16)
project(ralphEdisplay)

find_package(Qt6 REQUIRED COMPONENTS Quick Qml Network Multimedia)
qt_policy(SET QTP0001 NEW)

qt_add_executable(ralphEdisplay
    main.cpp
    backend.cpp
)

set_target_properties(ralphEdisplay PROPERTIES
    AUTOMOC ON
)

qt_add_qml_module(ralphEdisplay
    URI QTralphEdisplay
    VERSION 1.0
    QML_FILES main.qml
    RESOURCES
        fonts/JetBrainsMonoNerdFont-Regular.ttf
        sounds/alarm.wav
        icons/pooleIndustriesWhite.svg
)

target_link_libraries(ralphEdisplay PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::Multimedia
)
