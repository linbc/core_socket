file(GLOB sources_test *.cpp *.h *.c *.cc)
include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}/libs/include
  ${CMAKE_BINARY_DIR}/libs/deps  
  ${CMAKE_BINARY_DIR}/libs/deps/gtest/include
)
LINK_DIRECTORIES(
  ${CMAKE_BINARY_DIR}/libs/lib
)
add_executable(core_obj_test ${sources_test})
target_link_libraries(core_obj_test gtest core_obj)
