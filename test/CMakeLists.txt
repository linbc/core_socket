file(GLOB sources_test *.cpp *.h *.c *.cc)
add_executable(core_socket_test ${sources_test})
target_link_libraries(core_socket_test gtest core_socket)
