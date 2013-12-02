 
 
file(GLOB sources_localdir *.cpp *.h *.c *.cc)
file(GLOB sources_include ${CMAKE_SOURCE_DIR}/include/*.h)

set(src
  ${src}
  ${sources_localdir}
  ${sources_include}
)

add_definitions(-DTEA_EXPORTS)
add_library(core_socket STATIC ${src})

