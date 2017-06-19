cmake_minimum_required(VERSION 2.8)
set(_ECLIPSE_VERSION 4.6)
project(fdb)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules")
include(PlatformSpecificSupport)
include(GenerateCtags)

set(CMAKE_COLOR_MAKEFILE   ON)

option(WITH_COVERAGE "Build with code coverage support (requires lcov and build with tests)." OFF)
#option(WITH_DOC "Build API documentation (requires Doxygen)." OFF)
option(WITH_TESTS "Build tests (requires CppUTest)." OFF)

if(WITH_COVERAGE)
    set(WITH_TESTS ON)
    include(CodeCoverage)
    # Enable code coverage.
    # Build with debugging information to make the output meaningful.
    # Disable optimizations to get the most accurate results.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -g -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -g -O0")

    add_coverage(
        TARGET coverage
        TESTRUNNER ctest
        OUTPUTNAME coverage-report-fdb
        EXCLUDE '*/tests/*' '*/libdict/*'
    )
else()
    message(STATUS "COVERAGE disabled. ")
endif()
include(UseCppUTest)

# The version number
set (fdb_VERSION_MAJOR 0)
set (fdb_VERSION_MINOR 1)

# output directories
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(libdict)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)



set(CPACK_PACKAGE_NAME "fdb")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "FDB - An embedddable database")
set(CPACK_PACKAGE_DESCRIPTION           "FDB - An embedddable database with multiple modes of operation. It can be a kv-store, a persitable DB, a cache for remote data. It can be transactional, multi-key aware, composite key based etc.")

set(CPACK_RESOURCE_FILE_LICENSE         "${PROJECT_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README          "${PROJECT_SOURCE_DIR}/README.md")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER     "Shiva Velmurugan <shiv@shiv.me>")
set(CPACK_DEBIAN_PACKAGE_SECTION 		    "misc")
set(CPACK_DEBIAN_PACKAGE_PRIORITY       "extra")
set(CPACK_DEBIAN_PACKAGE_DEPENDS 		    "libc6")
set(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "debhelper (>= 8.0.0), cmake, g++ (>= 4.8)")

set(CPACK_PACKAGE_VENDOR 	              "shiv.me")
set(CPACK_PACKAGE_VERSION               "1.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR         "1")
set(CPACK_PACKAGE_VERSION_MINOR         "0")
set(CPACK_PACKAGE_VERSION_PATCH         "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY     "fdb_${CMAKE_PACKAGE_VERSION}")
set(CPACK_PACKAGE_EXECUTABLES           "simple" "simple")

set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE 1)

include(CPack)
