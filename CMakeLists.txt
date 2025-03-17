cmake_minimum_required(VERSION 3.0)
project(myMuduo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(LIBS 
    pthread
)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(examples)

enable_testing()