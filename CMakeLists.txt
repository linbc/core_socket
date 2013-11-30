project(core_obj)

set_property(GLOBAL PROPERTY USE_FOLDERS On)
cmake_minimum_required(VERSION 2.8)

set(PLATFORM X86)

if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(PLATFORM X64)
    MESSAGE(STATUS "Detected 64-bit platform")
else()
    MESSAGE(STATUS "Detected 32-bit platform")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-std=c++11")
  add_definitions(-fno-delete-null-pointer-checks)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lpthread")
endif()

if (WIN32)
  set(CMAKE_CPP_FLAGS "${CMAKE_C_FLAGS} /wd4819")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  
  add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)  
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)  
  add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)

  if(${PLATFORM} STREQUAL "X64")
    add_definitions("-D_WIN64")

	#Enable extended object support for debug compiles on X64 (not required on X86)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
  endif()
elseif(APPLE)
  MESSAGE("APPLE")
elseif(UNIX)
  message("UNIX")
endif()

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/out_libs)

add_subdirectory(src)
add_subdirectory(test)
