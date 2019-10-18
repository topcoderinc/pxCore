#!/bin/bash

cmake -DCMAKE_INSTALL_PREFIX=../extlibs/ .
make -j8
make install

