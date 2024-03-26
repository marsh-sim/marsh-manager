#! /bin/bash
# based on https://docs.appimage.org/packaging-guide/from-source/native-binaries.html#using-cmake-and-make-install

set -x
set -e

# building in temporary directory to keep system clean
# use RAM disk if possible (as in: not building on CI system like Travis, and RAM disk is available)
if [ "$CI" == "" ] && [ -d /dev/shm ]; then
    TEMP_BASE=/dev/shm
else
    TEMP_BASE=/tmp
fi

BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" appimage-build-XXXXXX)

# make sure to clean up build dir, even if errors occur
cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}
trap cleanup EXIT

# store repo root as variable
REPO_ROOT=$(readlink -f $(dirname $(dirname $0)))
OLD_CWD=$(readlink -f .)

# Setup Qt6 paths; requires Qt6_DIR variable, e.g. Qt6_DIR=/home/marek/Qt/6.5.3/gcc_64/
# works with default installation of 6.5.3
if [ -z "$Qt6_DIR" ]; then
    if [ -d "$HOME/Qt/6.5.3/gcc_64" ]; then
        Qt6_DIR="$HOME/Qt/6.5.3/gcc_64/"
    else
        echo "Set the Qt6_DIR variable to the desired Qt installation"
        exit 1
    fi
fi
export LD_LIBRARY_PATH=$Qt6_DIR/lib
export PATH=$Qt6_DIR/bin:$PATH

# switch to build dir
pushd "$BUILD_DIR"

# configure build files with CMake
# we need to explicitly set the install prefix, as CMake's default is /usr/local for some reason...
cmake "$REPO_ROOT" -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug

# build project and install files into AppDir
make -j$(nproc)
make install DESTDIR=AppDir

# make sure Qt plugin finds QML sources so it can deploy the imported files
export QML_SOURCES_PATHS="$REPO_ROOT"/src

# initialize AppDir, bundle shared libraries for QtQuickApp, use Qt plugin to bundle additional resources, and build AppImage, all in one single command
linuxdeploy --appdir AppDir --plugin qt --output appimage --desktop-file $REPO_ROOT/marsh-mgr.desktop --icon-file $REPO_ROOT/manager256.png

# move built AppImage back into original CWD
mv MARSH_Manager*.AppImage "$OLD_CWD"
