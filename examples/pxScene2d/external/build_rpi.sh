#!/bin/bash
set -e
# Any subsequent(*) commands which fail will cause the shell script to exit immediately

#--------- CURL 
cd curl

if [ "$(uname)" == "Darwin" ]; then
./configure --with-darwinssl
else
./configure
fi

make all -j3
cd ..

#--------- PNG 
cd png
./configure
make all -j3
cd ..

#--------- FT 

cd ft
export LIBPNG_LIBS="-L../png/.libs -lpng16"
./configure
make all -j3
cd ..

#--------- JPG 
cd jpg
./configure
make all -j3
cd ..

#--------- ZLIB 
cd zlib
./configure
make all -j3
cd ..


#--------- LIBNODE 
cd libnode-v6.9.0
./configure --shared
make -j3
ln -sf libnode.so.48 out/Release/obj.target/libnode.so
ln -sf libnode.48.dylib out/Release/libnode.dylib
cd ..

#-------- NANOSVG

cd nanosvg
quilt push -aq 
cd ..

