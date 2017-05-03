cmake_minimum_required(VERSION 2.8.7)
project(libtermkey)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 18)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(Unibilium REQUIRED)
include_directories(SYSTEM ${UNIBILIUM_INCLUDE_DIRS})

include_directories(${PROJECT_BINARY_DIR})

# Generate header termkey.h at configuration time.
# The t/ directory is a bit of a hack for the include directory, because tests
# expect to include ../termkey.h, but we are using out of source builds.
execute_process(
  COMMAND ${CMAKE_COMMAND}
  -DINPUT_FILE=${PROJECT_SOURCE_DIR}/termkey.h.in
  -DOUTPUT_FILE=${PROJECT_BINARY_DIR}/termkey.h.in
  -P ${PROJECT_SOURCE_DIR}/cmake/convert_in_file.cmake)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/t)
configure_file(${PROJECT_BINARY_DIR}/termkey.h.in ${PROJECT_BINARY_DIR}/termkey.h @ONLY)

add_definitions(-D _CRT_SECURE_NO_WARNINGS)
add_definitions(-DHAVE_UNIBILIUM)
if(NOT MSVC)
  add_definitions(-std=c99)
endif()

add_library(termkey termkey.c driver-csi.c driver-ti.c)
set_target_properties(termkey PROPERTIES PUBLIC_HEADER ${PROJECT_BINARY_DIR}/termkey.h
	VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")
target_link_libraries(termkey ${UNIBILIUM_LIBRARIES})

include(GNUInstallDirs)
install(TARGETS termkey
  PUBLIC_HEADER
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include_directories(${PROJECT_BINARY_DIR}/t)
enable_testing()
file(GLOB TESTSOURCES "t/[0-9]*.c")
foreach(f ${TESTSOURCES})
  get_filename_component(t ${f} NAME_WE)
  if(${t} STREQUAL 05read)
    continue()
  endif()

  add_executable("test_${t}" ${f} t/taplib.c)
  target_link_libraries("test_${t}" termkey)
  add_test("${t}" "test_${t}")
endforeach()
