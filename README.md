# FDB

key-value store / db 

[![coverage](https://img.shields.io/badge/coverage-report-blue)](https://friedsilicon.github.io/fdb/coverage/)
[![Join the chat at https://gitter.im/shiva/fdb](https://badges.gitter.im/shiva/fdb.svg)](https://gitter.im/shiva/fdb?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Trello](http://res.cloudinary.com/shiva/image/upload/c_scale,w_80/v1478231913/trello-logo-blue_fy6esb.png)](https://trello.com/b/MmkbCOA2)

## Compiling

To compile fdb, checkout this repo, initialize submodules and build, by executing the following commands

    # git submodule update --init --recursive
    # mkdir build
    # cd build
    # cmake ../

#### Build and Unit-testing

If you are contributing to the project, verify your change using unit-tests before commiting the change.

To enable testing, you can execute the following to generte the test targets

    # cmake -DWITH_TESTS=ON ../fdb/

To enable testing with coverage, execute

    # cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_COVERAGE=ON ../fdb

This project also supports building using the most awesome [ninja build system](https://ninja-build.org/).

    # cmake -G"Ninja" -DCMAKE_BUILD_TYPE=Debug -DWITH_COVERAGE=ON ../fdb

NOTE: This project uses Criterion for unit testing. Criterion is found via CMake's `find_package` mechanism.

## License

At the moment, all rights are reserved. This project will eventually be released with a dual-license. 


#### TODO

  - [x] Basic 2-phase txn manager
  - [ ] 2-phase commit implementation for db
  - [ ] Controller to track fdb's given out to client
  - [x] Serialization to disk
  - [ ] Support Pluggability (compile time selection of alternate implementations)?
  - [ ] Dual Licensing

