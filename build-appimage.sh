#!/bin/sh
rm -fr AppDir/
cmake -B build/ -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=AppDir/usr 
cmake --install build/ --config Release # main executable, icons and desktop file will be installed by cmake
linuxdeploy --appdir AppDir/  --output appimage
