# FDB

key-value store / db 

If you are using the version on github.com, you are using a [release-only mirror](https://github.com/friedsilicon/fdb). This project is developed and maintained at [friedsilicon/fdb](https://gitlab.com/friedsilicon/fdb) on gitlab.com.

[![build](https://gitlab.com/friedsilicon/fdb/badges/master/pipeline.svg)](https://gitlab.com/friedsilicon/fdb/commits/master)
[![coverage](https://gitlab.com/friedsilicon/fdb/badges/master/coverage.svg?job=build)](https://friedsilicon.gitlab.io/fdb/coverage/)
[![Join the chat at https://gitter.im/shiva/fdb](https://badges.gitter.im/shiva/fdb.svg)](https://gitter.im/shiva/fdb?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Trello](http://res.cloudinary.com/shiva/image/upload/c_scale,w_80/v1478231913/trello-logo-blue_fy6esb.png)](https://trello.com/b/MmkbCOA2)

## Compiling

To compile fdb, checkout this repo, initialize submodules and build, by executing the following commands

    # git submodule update --init --recursive
    # mkdir build
    # cd build
    # cmake ../

#### Generating Eclipse project

Note: build directory should be a sibling of where fdb is checked out. Eclipse doesn't like out-of-source builds that is a child of the source directory

    # git clone https://gitlab.com/friedsilicon/fdb.git fdb
    # cd fdb && git submodule update --init --recursive
    # cd ..
    # mkdir build && cd build
    # cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug ../fdb/

#### Build and Unit-testing

If you are contributing to the project, verify your change using unit-tests before commiting the change.

To enable testing, you can execute the following to generte the test targets

    # cmake -DWITH_TESTS=ON ../fdb/

To enable testing with coverage, execute

    # cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_COVERAGE=ON ../fdb

This project also supports building using the most awesome [ninja build system](https://ninja-build.org/).

    # cmake -G"Ninja" -DCMAKE_BUILD_TYPE=Debug -DWITH_COVERAGE=ON ../fdb

For the eclipse verion:

    # cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DWITH_TESTS=ON ../fdb/

NOTE: This project uses CPPUTEST for unit-testing. The cmake module for locating cpputest attempts to relies on CPPUTEST_DIR environment variable to locate the root of cpputest libraries and header files.

For example:

    # export CPPUTEST_DIR=/usr/local/Cellar/cpputest/3.8/

## License

At the moment, all rights are reserved. This project will eventually be released with a dual-license. 


#### TODO

  - [x] Basic 2-phase txn manager
  - [ ] 2-phase commit implementation for db
  - [ ] Controller to track fdb's given out to client
  - [x] Serialization to disk
  - [ ] Support Pluggability (compile time selection of alternate implementations)?
  - [ ] Dual Licensing

