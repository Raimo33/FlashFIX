cmake_minimum_required(VERSION 3.5)
project(flashfix VERSION 1.4.1 LANGUAGES C)

set(COMMON_COMPILE_OPTIONS
  -Wall
  -Wextra
  -Wpedantic
  -O3
  -march=native
  -lto
)

set(COMMON_COMPILE_DEFINITIONS _GNU_SOURCE)

add_library(flashfix_shared SHARED)
add_library(flashfix_static STATIC)
add_library(flashfix ALIAS flashfix_shared)

foreach(TARGET flashfix_shared flashfix_static)
  target_sources(${TARGET}
    PRIVATE
      src/deserializer.c
      src/serializer.c
      src/common.c
    PUBLIC
      FILE_SET HEADERS
      BASE_DIRS include
      FILES
        include/flashfix.h
        include/deserializer.h
        include/serializer.h
        include/structs.h
  )

  set_target_properties(${TARGET} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME flashfix
    C_STANDARD 23
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
  )
endforeach()

add_executable(test tests/test.c)
target_link_libraries(test PRIVATE flashfix_static)

add_executable(benchmark benchmarks/benchmark.c)
target_link_libraries(benchmark PRIVATE flashfix_static m)

foreach(TARGET test benchmark)
  set_target_properties(${TARGET} PROPERTIES
    C_STANDARD 23
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
  )
endforeach()

foreach(TARGET flashfix_shared flashfix_static test benchmark)
  target_compile_options(${TARGET} PRIVATE ${COMMON_COMPILE_OPTIONS})
  target_compile_definitions(${TARGET} PRIVATE ${COMMON_COMPILE_DEFINITIONS})
endforeach()

install(TARGETS flashfix_shared flashfix_static
  EXPORT flashfix-targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
  FILE_SET HEADERS DESTINATION include/flashfix
)