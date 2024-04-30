#!/bin/bash
mkdir cmake-build-emrelwithdebinfo
cd cmake-build-emrelwithdebinfo
emcmake cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../
#emcmake cmake -DCMAKE_BUILD_TYPE=Release ../
emmake make -j 16
cp graphs.js graphs.data graphs.wasm /var/www/html/playground/
