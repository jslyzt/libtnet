set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

file(GLOB_RECURSE
	SRC_FILES
	"*.cpp"
	"tests/*.cpp"
)

add_executable(test ${GLOB_RECURSE})
target_link_libraries(test
	-tnet
	-lgmock
)