enable_language(C)

add_definitions(-std=c99)

set(FDB_SOURCES
    common/log.c
    db/db.c
    db/db_serialize.c
    txn/txn.c
)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/libdict/include)

add_library(fdb SHARED ${FDB_SOURCES})
target_link_libraries(fdb dict)

install(
	TARGETS fdb
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	COMPONENT fdb)

install(
	DIRECTORY 	${PROJECT_SOURCE_DIR}/include/fdb
	DESTINATION include
	COMPONENT 	fdb)
