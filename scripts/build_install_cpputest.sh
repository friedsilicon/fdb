#!/usr/bin/env bash

git clone https://github.com/cpputest/cpputest.git --depth 1 cpputest-src
mkdir -p cpputest-build
cd cpputest-build
cmake ../cpputest-src
make && make install
cd ..
