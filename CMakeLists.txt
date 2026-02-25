cmake_minimum_required(VERSION 3.21)

project(updated_device_driver VERSION 0.1.0 LANGUAGES C CXX)
file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" DEVICE_DRIVER_SOURCE_ROOT)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DEVICE_DRIVER_BUILD_TESTS "Build unit tests" ON)
option(DEVICE_DRIVER_BUILD_DOCS "Generate API documentation with Doxygen" ON)
option(DEVICE_DRIVER_ENABLE_COVERAGE "Enable code coverage instrumentation (GCC/Clang only)" OFF)

if(DEVICE_DRIVER_ENABLE_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Code coverage is only supported with GCC or Clang.")
    endif()
    if(NOT DEVICE_DRIVER_BUILD_TESTS)
        message(FATAL_ERROR "Code coverage requires DEVICE_DRIVER_BUILD_TESTS=ON.")
    endif()

    add_compile_options(-O0 -g --coverage)
    add_link_options(--coverage)
endif()

add_library(device_driver
    src/device_driver.c
    src/device_driver_tx.c
    src/device_driver_rx.c
    src/registers.c
    src/queue.c
)
add_library(device_driver::device_driver ALIAS device_driver)

target_include_directories(device_driver
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(DEVICE_DRIVER_BUILD_DOCS)
    find_package(Doxygen QUIET)
    if(Doxygen_FOUND)
        set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_INPUT_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        )
        string(REPLACE ";" " " DOXYGEN_INPUT "${DOXYGEN_INPUT_DIRS}")

        configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
            ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            @ONLY
        )

        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    else()
        message(STATUS "Doxygen not found. 'docs' target will not be available.")
    endif()
endif()

if(DEVICE_DRIVER_BUILD_TESTS)
    include(FetchContent)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(device_driver_tests
        tests/test_device_driver.cpp
        tests/test_queue.cpp
    )

    target_link_libraries(device_driver_tests
        PRIVATE
            device_driver::device_driver
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(device_driver_tests)

    if(DEVICE_DRIVER_ENABLE_COVERAGE)
        find_program(GCOVR_EXECUTABLE gcovr)
        if(NOT GCOVR_EXECUTABLE)
            message(FATAL_ERROR "DEVICE_DRIVER_ENABLE_COVERAGE=ON requires gcovr in PATH.")
        endif()
        find_program(FIND_EXECUTABLE find)
        if(NOT FIND_EXECUTABLE)
            message(FATAL_ERROR "DEVICE_DRIVER_ENABLE_COVERAGE=ON requires 'find' in PATH.")
        endif()

        add_custom_target(coverage
            COMMAND ${FIND_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR} -name "*.gcda" -delete
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${GCOVR_EXECUTABLE}
                --root ${DEVICE_DRIVER_SOURCE_ROOT}
                --object-directory ${CMAKE_CURRENT_BINARY_DIR}
                --gcov-ignore-errors=no_working_dir_found
                --filter "${DEVICE_DRIVER_SOURCE_ROOT}/src/"
                --filter "${DEVICE_DRIVER_SOURCE_ROOT}/include/"
                --exclude ".*/_deps/.*"
                --exclude-directories ${CMAKE_CURRENT_BINARY_DIR}/_deps
                --print-summary
                --xml-pretty --output coverage.xml
                --html-details coverage.html
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS device_driver_tests
            COMMENT "Running tests and generating coverage reports"
            VERBATIM
        )
    endif()
endif()

add_executable(device_driver_test_main
    tests/device_driver_test_main.c
)
target_link_libraries(device_driver_test_main
    PRIVATE
        device_driver::device_driver
)

# Packaging
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "TGZ" CACHE STRING "CPack generator to use (e.g. TGZ;ZIP;DEB;RPM)")
include(CPack)
