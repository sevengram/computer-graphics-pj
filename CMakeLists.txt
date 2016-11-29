cmake_minimum_required(VERSION 2.8)
project(dof)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

link_libraries(${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} m)

SET(LIB_SRC errcheck.c fatal.c loadtexbmp.c object.c print.c project.c)
add_library(CSCIx229 STATIC ${LIB_SRC})

link_libraries(CSCIx229)
add_executable(dof main.c)