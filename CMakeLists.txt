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
  src/internal/common.c
)

set(FLASHFIX_PUBLIC_HEADERS
  include/flashfix/deserializer.h
  include/flashfix/serializer.h
)

add_library(flashfix_shared SHARED ${FLASHFIX_SOURCES})
target_include_directories(flashfix_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(flashfix_shared PROPERTIES
  VERSION ${FLASHFIX_VERSION}
  SOVERSION ${FLASHFIX_VERSION_MAJOR}
  OUTPUT_NAME flashfix
)
target_compile_options(flashfix_shared PUBLIC
  -Wall -Wextra -Werror -Wpedantic
  -fwrapv -fno-strict-aliasing -Ofast -march=native
  -fno-plt -fno-stack-protector -fno-semantic-interposition
  -fno-asynchronous-unwind-tables -fomit-frame-pointer -funroll-loops
)


add_library(flashfix_static STATIC ${FLASHFIX_SOURCES})
target_include_directories(flashfix_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(flashfix_static PROPERTIES
  OUTPUT_NAME flashfix
)
target_compile_options(flashfix_static PUBLIC
  -Wall -Wextra -Werror -Wpedantic
  -fwrapv -fno-strict-aliasing -Ofast -march=native
  -fno-plt -fno-stack-protector -fno-semantic-interposition
  -fno-asynchronous-unwind-tables -fomit-frame-pointer -funroll-loops
)


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

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests_ran
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_static
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_shared
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/tests_ran
  DEPENDS test_static test_shared
  COMMENT "Running tests with both static and shared libraries"
)

add_custom_target(run_tests ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tests_ran
  COMMENT "Tests have been run"
)

add_custom_target(tests DEPENDS run_tests)