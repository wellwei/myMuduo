file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# 创建静态库或动态库
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

# 添加头文件路径
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)

# 为 Debug 模式启用 MUDEBUG
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(myMuduo PRIVATE MUDEBUG)
endif()
