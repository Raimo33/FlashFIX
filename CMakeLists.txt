cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER gcc)
project(flashfix LANGUAGES C VERSION 1.0.0)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(FLASHFIX_VERSION_MAJOR 1)
set(FLASHFIX_VERSION_MINOR 0)
set(FLASHFIX_VERSION_PATCH 0)
set(FLASHFIX_VERSION "${FLASHFIX_VERSION_MAJOR}.${FLASHFIX_VERSION_MINOR}.${FLASHFIX_VERSION_PATCH}")

set(FLASHFIX_SOURCES
  src/deserializer.c
  src/serializer.c
  src/common.c
)

set(FLASHFIX_PUBLIC_HEADERS
  include/flashfix.h
  include/flashfix/deserializer.h
  include/flashfix/serializer.h
)

set(FLASHFIX_COMPILE_OPTIONS
  -Wall -Wextra -Werror -Wpedantic
  -fwrapv -fno-strict-aliasing -Ofast -march=native
  -fno-semantic-interposition -fno-stack-protector -fno-plt 
  -fno-asynchronous-unwind-tables -funroll-loops -fomit-frame-pointer
)

add_library(flashfix_shared SHARED ${FLASHFIX_SOURCES})
target_include_directories(flashfix_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(flashfix_shared PROPERTIES
  VERSION ${FLASHFIX_VERSION}
  SOVERSION ${FLASHFIX_VERSION_MAJOR}
  OUTPUT_NAME flashfix
)
target_compile_options(flashfix_shared PUBLIC ${FLASHFIX_COMPILE_OPTIONS})

add_library(flashfix_static STATIC ${FLASHFIX_SOURCES})
target_include_directories(flashfix_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(flashfix_static PROPERTIES
  OUTPUT_NAME flashfix
)
target_compile_options(flashfix_static PUBLIC ${FLASHFIX_COMPILE_OPTIONS})

install(TARGETS flashfix_shared flashfix_static
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

install(FILES ${FLASHFIX_PUBLIC_HEADERS}
  DESTINATION include/flashfix
)

add_executable(test_static tests/test.c)
target_link_libraries(test_static flashfix_static)

add_executable(test_shared tests/test.c)
target_link_libraries(test_shared flashfix_shared)

add_executable(benchmark_static tests/benchmark.c)
target_link_libraries(benchmark_static flashfix_static m)
target_compile_options(benchmark_static PUBLIC ${FLASHFIX_COMPILE_OPTIONS} -flto -g)

add_executable(benchmark_shared tests/benchmark.c)
target_link_libraries(benchmark_shared flashfix_static m)
target_compile_options(benchmark_shared PUBLIC ${FLASHFIX_COMPILE_OPTIONS} -flto -g)

add_custom_target(run_tests ALL
  COMMAND $<TARGET_FILE:test_static>
  COMMAND $<TARGET_FILE:test_shared>
  COMMENT "Running all tests (CTest completely disabled)"
)

add_custom_target(run_benchmarks
  COMMAND $<TARGET_FILE:benchmark_static>
  COMMAND $<TARGET_FILE:benchmark_shared>
  COMMENT "Running all benchmarks"
)