set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

file(GLOB
	TNET_FILES
	"*.cpp"
	"http/*.c"
	"http/*.cpp"
	"polarssl/*.c"
)
add_library(tnet ${TNET_FILES})


file(GLOB
	REDIS_FILES
	"redis/*.c"
	"redis/*.cpp"
)
add_library(tredis ${REDIS_FILES})