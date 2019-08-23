set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

file(GLOB
	SRC_FILES
	"*.cpp"
	"tests/*.cpp"
)

add_executable(test ${SRC_FILES})
target_link_libraries(test
	-ltnet
	-lgmock
	-lpthread
)