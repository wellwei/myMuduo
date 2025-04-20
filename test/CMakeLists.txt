set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Add test executable
file(GLOB TEST_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# 链接Google Test
find_package(GTest CONFIG REQUIRED)

add_executable(tests ${TEST_SRC_FILES})
target_link_libraries(tests GTest::gtest GTest::gtest_main myMuduo)

# 添加UNIT_TEST宏定义
target_compile_definitions(tests PRIVATE UNIT_TEST)

# Add test
add_test(NAME tests COMMAND tests)

# 添加编译选项
target_compile_options(tests PRIVATE -g -O2)

