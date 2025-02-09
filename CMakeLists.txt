cmake_minimum_required(VERSION 3.10)

project(my_library VERSION 1.0.0 LANGUAGES C)

add_library(my_library
  src/common.c
  src/deserializer.c
  src/serializer.c
)

target_include_directories(my_library PUBLIC include)
target_include_directories(my_library PRIVATE src/)

install(DIRECTORY include/my_library DESTINATION include)
install(TARGETS my_library DESTINATION lib)