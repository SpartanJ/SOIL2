cmake_minimum_required(VERSION 3.9)
project(soil2 LANGUAGES CXX C VERSION 1.0.0)

option(SOIL2_BUILD_TESTS "Build tests")

find_package(OpenGL REQUIRED)

add_library(soil2
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/image_DXT.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/image_helper.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/SOIL2.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/wfETC.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/common.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/pkm_helper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/pvr_helper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stb_image.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stb_image_write.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stbi_DDS.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stbi_DDS_c.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stbi_ext.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stbi_ext_c.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stbi_pkm.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stbi_pkm_c.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stbi_pvr.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/stbi_pvr_c.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/wfETC.h"
)

target_compile_options(soil2 PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-fPIC>
    $<$<CXX_COMPILER_ID:GNU>:-fPIC>
)

target_include_directories(soil2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/common/>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(soil2 PRIVATE OpenGL::GL)

set_property(TARGET soil2 PROPERTY PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/SOIL2.h)

if(SOIL2_BUILD_TESTS)
    find_package(SDL2 REQUIRED)

    add_executable(soil2_test
        ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_SOIL2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/common.cpp
    )
    target_link_libraries(soil2_test soil2 SDL2::SDL2 OpenGL::GL)

    add_executable(soil2_perf_test
        ${CMAKE_CURRENT_SOURCE_DIR}/src/perf_test/test_perf_SOIL2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/common.cpp
    )
    target_link_libraries(soil2_perf_test soil2 SDL2::SDL2 OpenGL::GL)

    # Create symlink to test images
    add_custom_command(
        TARGET soil2_test PRE_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_directory
       ${CMAKE_SOURCE_DIR}/bin/ $<TARGET_FILE_DIR:soil2_test>
    )
endif()

install(
    TARGETS soil2 
    EXPORT soil2_target
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

if(NOT SOIL2_SKIP_HEADERS)
    install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/SOIL2/SOIL2.h
        DESTINATION include/soil2
    )
endif()

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/soil2-config.cmake.in
    soil2-config.cmake
    INSTALL_DESTINATION share/soil2
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/soil2-config.cmake DESTINATION share/soil2)

install(
    EXPORT soil2_target
    NAMESPACE soil2::
    FILE soil2-targets.cmake
    DESTINATION share/soil2
)
