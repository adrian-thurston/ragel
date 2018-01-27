cmake_minimum_required(VERSION 3.0)

project(colm
	VERSION 0.13.0.5
	LANGUAGES C CXX)
set(PROJECT_PUBDATE "May 2017")

set(${PROJECT_NAME}_MAKE_INSTALL ON CACHE BOOL
	"Set to OFF to disable install rules (default is ON)")

add_subdirectory(src)
