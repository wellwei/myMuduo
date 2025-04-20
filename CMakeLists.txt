cmake_minimum_required(VERSION 3.0)

set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux)

project(myMuduo VERSION 1.0.0 LANGUAGES CXX)

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

# 配置安装
include(GNUInstallDirs)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/myMuduo
        FILES_MATCHING PATTERN "*.h"
)

install(TARGETS myMuduo
        EXPORT myMuduoTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT myMuduoTargets
        FILE myMuduoTargets.cmake
        NAMESPACE myMuduo::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/myMuduo
)

# 配置 CMake 包
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/myMuduoConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/myMuduoConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/myMuduo
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/myMuduoConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/myMuduo
)