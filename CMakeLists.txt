cmake_minimum_required(VERSION 3.18) # REQUIRED em find_library/find_path exige >= 3.18
project(smith_chart_pro VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg PrintSupport)

# Busca pela biblioteca ONNX Runtime no sistema
find_library(ONNXRUNTIME_LIB onnxruntime REQUIRED)
find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h REQUIRED)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/smith_chart_canvas.cpp
    src/main_window.cpp
    src/s1p_parser.cpp
    src/ai_engine.cpp
)

set(HEADERS
    src/smith_chart_canvas.h
    src/main_window.h
    src/s1p_parser.h
    src/types.h
    src/ai_engine.h
)

qt_add_executable(smith_chart_pro ${SOURCES} ${HEADERS})

target_include_directories(smith_chart_pro PRIVATE ${ONNXRUNTIME_INCLUDE_DIR})

target_link_libraries(smith_chart_pro PRIVATE 
    Qt6::Widgets 
    Qt6::Svg 
    Qt6::PrintSupport
    ${ONNXRUNTIME_LIB}
)