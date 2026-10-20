include(UseCMocka)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/src/storage)
include_directories(${PROJECT_SOURCE_DIR}/libdict/include)

if(WITH_TESTS)
    add_executable(test-dict-lookup ../libdict/lookup_tests.c)
    target_link_libraries(test-dict-lookup PRIVATE dict)
    add_test(NAME test-dict-lookup COMMAND test-dict-lookup)
endif()

add_unit_test(
    TARGET fdb-init
    SOURCES fdbInitTests.c
            ../src/common/log.c
            ../src/db/db.c
            ../src/storage/mem_backend.c
    LIBRARIES dict
)

add_unit_test(
    TARGET fdb-ops
    SOURCES fdbOpsTests.c
            ../src/common/log.c
            ../src/db/db.c
            ../src/storage/mem_backend.c
    LIBRARIES dict
)

add_unit_test(
    TARGET fdb-txn
    SOURCES fdbTxnTests.c
            ../src/common/log.c
            ../src/db/db.c
            ../src/txn/txn.c
            ../src/storage/mem_backend.c
    LIBRARIES dict
)

add_unit_test(
    TARGET fdb-serialize
    SOURCES fdbSerializeTests.c
            ../src/common/log.c
            ../src/db/db.c
            ../src/db/db_serialize.c
            ../src/txn/txn.c
            ../src/storage/mem_backend.c
    LIBRARIES dict
)
