include(cmake/ProjectGuard.cmake)
cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(
  lcrypt
  VERSION "0.1.0"
  LANGUAGES CXX C)

# https://cmake.org/cmake/help/latest/variable/PROJECT_IS_TOP_LEVEL.html
string(COMPARE EQUAL ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR} PROJECT_IS_TOP_LEVEL)

option(LCRYPT_INSTALL "Enable install" ${PROJECT_IS_TOP_LEVEL})
option(LCRYPT_BUILD_TESTS "Build unittest" ${PROJECT_IS_TOP_LEVEL})
option(LCRYPT_BUILD_BENCHES "Build benchmark" ${PROJECT_IS_TOP_LEVEL})

#####################################
# compile & link options
#####################################
include(cmake/common.cmake)

include(CheckFunctionExists)
check_function_exists(memmem LC_HAS_MEMMEM)

include(TestBigEndian)
test_big_endian(LC_IS_BIG_ENDIAN)
message(STATUS "big endian: ${LC_IS_BIG_ENDIAN}")

#####################################
# third-party packages
#####################################
include(cmake/cpm.cmake)

if (LCRYPT_INSTALL)
  CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.13.0")
endif()

if(LCRYPT_BUILD_TESTS)
  CPMAddPackage("gh:google/googletest#v1.15.2")
endif()

if(LCRYPT_BUILD_BENCHES)
  CPMAddPackage("gh:martinus/nanobench#v4.3.11")
endif()

# highway
CPMAddPackage(
  NAME highway
  URL https://github.com/google/highway/archive/refs/tags/1.2.0.tar.gz
  URL_HASH SHA256=7e0be78b8318e8bdbf6fa545d2ecb4c90f947df03f7aadc42c1967f019e63343
  PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/highway.patch" # This adds SYSTEM to the includes.
  OPTIONS
    "HWY_ENABLE_EXAMPLES OFF"
    "HWY_ENABLE_INSTALL ON"
    "HWY_ENABLE_TESTS OFF"
    "HWY_ENABLE_CONTRIB OFF"
)

# check flag
if (MSVC)
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2)
  if (COMPILER_SUPPORTS_AVX2)
    message(STATUS "MSVC compiler support AVX2")
    target_compile_options(hwy PUBLIC "/arch:AVX2")
  endif()
endif()

#####################################
# target
#####################################
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp")

add_library(${PROJECT_NAME} STATIC ${headers} ${sources})
target_link_libraries(${PROJECT_NAME} PRIVATE hwy)
target_compile_definitions(
  ${PROJECT_NAME} PRIVATE
    $<$<BOOL:${LC_IS_BIG_ENDIAN}>:LC_IS_BIG_ENDIAN>
    $<$<BOOL:${LC_HAS_MEMMEM}>:LC_HAS_MEMMEM>)
target_include_directories(
  ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>)

#####################################
# test
#####################################
if(LCRYPT_BUILD_TESTS) #  AND NOT CMAKE_CROSSCOMPILING)
  enable_testing()
  add_subdirectory(tests)
endif()

#####################################
# benchmark
#####################################
if(LCRYPT_BUILD_BENCHES)
   add_subdirectory(benchmarks)
endif()

#####################################
# install & pack
#####################################
if(LCRYPT_INSTALL)
  packageProject(
    NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION} NAMESPACE ${PROJECT_NAME}
    BINARY_DIR ${PROJECT_BINARY_DIR}
    INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
    INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )
endif()

