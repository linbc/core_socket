 
 
file(GLOB sources_localdir *.cpp *.h *.c)
file(GLOB sources_include ${CMAKE_SOURCE_DIR}/include/*.h)

set(core_obj_STAT_SRCS
  ${core_obj_STAT_SRCS}
  ${sources_localdir}
  ${sources_include}
)

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

add_definitions(-DTEA_EXPORTS)
add_library(core_obj STATIC ${core_obj_STAT_SRCS})
#target_link_libraries(core_obj net comm)

