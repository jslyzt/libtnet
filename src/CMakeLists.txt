set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

file(GLOB_RECURSE
	SRC_FILES
	"*.cpp"
	"http/*.c"
	"http/*.cpp"
	"polarssl/*.c"
	"redis/*.c"
	"redis/*.cpp"
)

add_library(tnet ${SRC_FILES})