cmake_minimum_required(VERSION 3.21)

project(updated_serial_driver VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SERIAL_DRIVER_BUILD_TESTS "Build unit tests" ON)
option(SERIAL_DRIVER_BUILD_DOCS "Generate API documentation with Doxygen" ON)
option(SERIAL_DRIVER_ENABLE_COVERAGE "Enable code coverage instrumentation (GCC/Clang only)" OFF)

if(SERIAL_DRIVER_ENABLE_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Code coverage is only supported with GCC or Clang.")
    endif()
    if(NOT SERIAL_DRIVER_BUILD_TESTS)
        message(FATAL_ERROR "Code coverage requires SERIAL_DRIVER_BUILD_TESTS=ON.")
    endif()

    add_compile_options(-O0 -g --coverage)
    add_link_options(--coverage)
endif()

add_library(serial_driver
    src/serial_driver.c
    src/registers.c
)
add_library(serial_driver::serial_driver ALIAS serial_driver)

target_include_directories(serial_driver
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(SERIAL_DRIVER_BUILD_DOCS)
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

if(SERIAL_DRIVER_BUILD_TESTS)
    include(FetchContent)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(serial_driver_tests
        tests/test_serial_driver.cpp
    )

    target_link_libraries(serial_driver_tests
        PRIVATE
            serial_driver::serial_driver
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(serial_driver_tests)

    if(SERIAL_DRIVER_ENABLE_COVERAGE)
        find_program(GCOVR_EXECUTABLE gcovr)
        if(GCOVR_EXECUTABLE)
            add_custom_target(coverage ALL
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${GCOVR_EXECUTABLE}
                    --root ${CMAKE_CURRENT_SOURCE_DIR}
                    --object-directory ${CMAKE_CURRENT_BINARY_DIR}
                    --filter ${CMAKE_CURRENT_SOURCE_DIR}/src
                    --filter ${CMAKE_CURRENT_SOURCE_DIR}/include
                    --exclude ".*/_deps/.*"
                    --exclude-directories ${CMAKE_CURRENT_BINARY_DIR}/_deps
                    --print-summary
                    --xml-pretty --output coverage.xml
                    --html-details coverage.html
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                DEPENDS serial_driver_tests
                COMMENT "Running tests and generating coverage reports"
            )
        else()
            message(FATAL_ERROR "SERIAL_DRIVER_ENABLE_COVERAGE=ON requires gcovr in PATH.")
        endif()
    endif()
endif()

# Packaging
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_GENERATOR "TGZ" CACHE STRING "CPack generator to use (e.g. TGZ;ZIP;DEB;RPM)")
include(CPack)
