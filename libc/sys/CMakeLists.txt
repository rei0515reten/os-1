
add_library(sys STATIC
	write.S)

target_include_directories(sys PUBLIC
	${PROJECT_SOURCE_DIR}/libc/include
	# TODO:
	${PROJECT_SOURCE_DIR}/include)
