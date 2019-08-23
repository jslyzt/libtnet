set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

file(GLOB
	TNET_FILES
	"*.cpp"
	"http/*.c"
	"http/*.cpp"
)
add_library(tnet ${TNET_FILES})


file(GLOB
	SSL_FILES
	"polarssl/*.c"
)
add_library(polarssl ${SSL_FILES})

file(GLOB
	REDIS_FILES
	"redis/*.c"
	"redis/*.cpp"
)
add_library(redis ${SRC_FILES})