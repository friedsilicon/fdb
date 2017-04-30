include(UseCppUTest)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/libdict/include)

add_unit_test(
    TARGET fdb
    SOURCES fdbTests.cpp
            fdbTxnTests.cpp
            RunAllTests.cpp
            ../src/common/log.c
            ../src/db/db.c
            ../src/txn/txn.c
    LIBRARIES dict

)
