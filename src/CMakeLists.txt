file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# 创建静态库或动态库
add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

# 添加头文件路径
target_include_directories(${PROJECT_NAME} 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 为 Debug 模式启用 MUDEBUG
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(myMuduo PRIVATE MUDEBUG)
endif()

# 设置版本号
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
