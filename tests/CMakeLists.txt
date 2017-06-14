include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/libdict/include)

add_unit_test(
    TARGET fdb-ops
    SOURCES fdbInitTests.cpp
            fdbOpsTests.cpp
            RunAllTests.cpp
            ../src/common/log.c
            ../src/db/db.c
    LIBRARIES dict
)

add_unit_test(
    TARGET fdb-txn
    SOURCES fdbInitTests.cpp
            fdbOpsTests.cpp
            fdbTxnTests.cpp
            RunAllTests.cpp
            ../src/common/log.c
            ../src/db/db.c
            ../src/txn/txn.c
    LIBRARIES dict

)

add_unit_test(
    TARGET fdb-serialize
    SOURCES fdbInitTests.cpp
            fdbOpsTests.cpp
            fdbSerializeTests.cpp
            RunAllTests.cpp
            ../src/common/log.c
            ../src/db/db.c
            ../src/db/db_serialize.c
            ../src/txn/txn.c
    LIBRARIES dict
)
