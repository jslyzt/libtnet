cmake_minimum_required(VERSION 2.8)

project(TNET)

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(LFT_YUN_LIBS $ENV{LFT_YUN_LIBS})

include_directories(
	${LFT_YUN_LIBS}/l-include
	${LFT_YUN_LIBS}/include
	${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/src/http
	${PROJECT_SOURCE_DIR}/src/polarssl
	${PROJECT_SOURCE_DIR}/src/redis
	${PROJECT_SOURCE_DIR}/test
)

link_directories(
	${LIBRARY_OUTPUT_PATH}
	${LFT_YUN_LIBS}/libs/linux
)

add_subdirectory(src)
add_subdirectory(test)
