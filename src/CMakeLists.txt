file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# 创建静态库或动态库
add_library(myMuduo SHARED ${SRC_FILES})

# 设置库的导出名称
set_target_properties(myMuduo PROPERTIES
    OUTPUT_NAME "myMuduo"
    VERSION 1.0.0
    SOVERSION 1
)

# 添加头文件路径
target_include_directories(myMuduo 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/myMuduo>
)

# 链接pthread库
target_link_libraries(myMuduo PRIVATE ${LIBS})

# 为 Debug 模式启用 MUDEBUG
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(myMuduo PRIVATE MUDEBUG)
endif()
