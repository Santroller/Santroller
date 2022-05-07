#!/bin/bash
cd build
cmake .. -DBOARD=
make -j`nproc`


for filename in ../submodules/pico-sdk/src/boards/include/boards/*.h; do
    filename=`basename -s .h ${filename}`
    if [ "$filename" != "none" ]; then
        cmake .. -DBOARD=${filename}
        make -j`nproc`
    fi
done
cmake .. -DBOARD=