include(UseCppUTest)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/libdict/include)

add_executable(
    simple
    simple.cpp
)
target_link_libraries(simple fdb)

install(
	TARGETS simple
	RUNTIME DESTINATION bin)