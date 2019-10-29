#!/bin/bash

git clone https://github.com/google/shaka-player.git
cd shaka-player
git checkout c014d445a4c73237c5f530ddd67cef4d4e52aad4
git apply < ../0001-shaka-initial-porting.patch
python build/all.py --debug
sed -i 's/b.f.a(/b.f.attachMediaSource(/' dist/shaka-player.compiled.debug.js
