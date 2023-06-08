#!/bin/bash
cd build
cmake .. -DBOARD=
make -j`nproc`