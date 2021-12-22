#######################################################################################################################
# Set the minimum required version of cmake
#######################################################################################################################

cmake_minimum_required(VERSION 3.20.1)

#######################################################################################################################
# Set compiler
#######################################################################################################################

message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_C_COMPILER_ID: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

#######################################################################################################################
# Set project environment
#######################################################################################################################

project(HashTableBenchmark)
include_directories(${PROJECT_SOURCE_DIR})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CACHEFILE_DIR "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/bin")

message(STATUS "CMAKE_ROOT: ${CMAKE_ROOT}")
message(STATUS "CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Python3_SITELIB: ${Python3_SITELIB}")

#######################################################################################################################
# Selecting a modern standard version
#######################################################################################################################

# Export the compilation report, that can be later be reused by static 
# analysis tools and other apps like SourceTrail.
set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
  message(STATUS "Build type not specified: Use RELEASE by default")
endif()

if(CMAKE_BUILD_TYPE EQUAL "RELEASE")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_UNITY_BUILD ON)
endif()

#######################################################################################################################
# Macros and additional flags
#######################################################################################################################

# General flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wreorder -Wunused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unknown-pragmas")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wattributes")

# Debugging flags.
# https://code.visualstudio.com/docs/cpp/faq-cpp#_how-do-i-enable-debug-symbols
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG_SAFE_MODE -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG_SAFE_MODE -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -O2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O2")

# Compiler-specific flags.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fstrict-vtable-pointers")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ferror-limit=1")
  # Riscky Performance Optimization, that require tuning.
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -momit-leaf-frame-pointer")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -funsafe-math-optimizations")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wc++11-narrowing")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=1")  
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
else ()
  message(STATUS "Unknown CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
endif()

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#######################################################################################################################
# Build tests benchmarks
#######################################################################################################################

include_directories(src)
include(FetchContent)

FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(googlebenchmark GIT_REPOSITORY https://github.com/google/benchmark)
FetchContent_MakeAvailable(googlebenchmark)

set(CXX_TARGET_LINK_LIBRARIES gtest_main benchmark::benchmark)

add_executable(ram_speed src/ram_speed_bench.cxx)
add_executable(ram_usage src/ram_usage_bench.cxx)
add_executable(mixed_ops src/mixed_ops_bench.cxx)

target_link_libraries(ram_speed${ARTIFACT_SUFFIX} 
  ${CXX_TARGET_LINK_LIBRARIES}
  
  ${CXX_TARGET_LINK_LIBRARIES_WITH_FLAGS}
  ${PKG_PACKAGES_LIBRARIES_WITH_FLAGS}
)

target_link_libraries(ram_usage${ARTIFACT_SUFFIX} 
  ${CXX_TARGET_LINK_LIBRARIES}
  
  ${CXX_TARGET_LINK_LIBRARIES_WITH_FLAGS}
  ${PKG_PACKAGES_LIBRARIES_WITH_FLAGS}
)

target_link_libraries(mixed_ops${ARTIFACT_SUFFIX} 
  ${CXX_TARGET_LINK_LIBRARIES}
  
  ${CXX_TARGET_LINK_LIBRARIES_WITH_FLAGS}
  ${PKG_PACKAGES_LIBRARIES_WITH_FLAGS}
)
