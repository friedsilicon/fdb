include(UseCMocka)
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/libdict/include)

add_unit_test(
    TARGET fdb-init
    SOURCES fdbInitTests.c
            ../src/common/log.c
            ../src/db/db.c
    LIBRARIES dict
)

add_unit_test(
    TARGET fdb-ops
    SOURCES fdbOpsTests.c
            ../src/common/log.c
            ../src/db/db.c
    LIBRARIES dict
)

add_unit_test(
    TARGET fdb-txn
    SOURCES fdbTxnTests.c
            ../src/common/log.c
            ../src/db/db.c
            ../src/txn/txn.c
    LIBRARIES dict
)

add_unit_test(
    TARGET fdb-serialize
    SOURCES fdbSerializeTests.c
            ../src/common/log.c
            ../src/db/db.c
            ../src/db/db_serialize.c
            ../src/txn/txn.c
    LIBRARIES dict
)
