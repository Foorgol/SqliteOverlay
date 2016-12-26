#!/bin/bash

PKGVER=0.3.2
PKGRELEASE=1
PKGNAME=SqliteOverlay
BUILDDIR=/tmp/$PKGNAME-build

STARTDIR=$(pwd)

mkdir $BUILDDIR
pushd $BUILDDIR

git clone -b $PKGVER --single-branch --depth 1 https://github.com/Foorgol/SqliteOverlay.git
cd SqliteOverlay
mkdir release
cd release
cmake .. -DCMAKE_BUILD_TYPE=Release
make
echo "C++ lib for easier interfacing with SQLite databasses" > description-pak
checkinstall --pkgversion $PKGVER \
	--pkgrelease $PKGRELEASE \
	--pkgname $PKGNAME \
	--provides $PKGNAME \
	--install=no \
	--nodoc \
	-y

mv *deb $STARTDIR

popd

rm -rf $BUILDDIR

