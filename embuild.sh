#!/bin/bash
cd cmake-build-emrelwithdebinfo
emcmake cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../
emmake make -j 16
cp graphs.js graphs.wasm /var/www/html
