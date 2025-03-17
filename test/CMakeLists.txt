# Enable testing
enable_testing()

# Add test executable
file(GLOB TEST_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# 链接Google Test
find_package(GTest REQUIRED)

add_executable(tests ${TEST_SRC_FILES})
target_link_libraries(tests GTest::GTest GTest::Main myMuduo)

# 添加UNIT_TEST宏定义
target_compile_definitions(tests PRIVATE UNIT_TEST)

# Add test
add_test(NAME tests COMMAND tests)

