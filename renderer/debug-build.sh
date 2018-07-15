#!/bin/bash

if [ `uname` = 'Darwin' ]; then
	IS_MAC="1"
	LDD="otool -L"
else
	IS_MAC=""
	LDD="ldd"
fi
SOFILE=../House3D/objrender.so
BINFILE=objview.bin

###### ENV VARS:
echo "PATH:
$PATH"
echo
echo "CPATH:
$CPATH"
echo
echo "PKG_CONFIG_PATH:
$PKG_CONFIG_PATH"
echo
echo "LIBRARY_PATH:
$LIBRARY_PATH"
echo
echo "LD_LIBRARY_PATH:
$LD_LIBRARY_PATH"
echo
echo "PYTHONPATH:
$PYTHONPATH"
echo
PYCONFIG=${PYTHON_CONFIG:-python-config}

##### Flags:
set -x
pkg-config --cflags --libs glfw3

if [ ! "$IS_MAC" ]; then
	pkg-config --cflags --libs libglvnd
	pkg-config --cflags --libs egl
fi

which find
which pkg-config
which $PYCONFIG
$PYCONFIG --includes --ldflags
if [ $IS_MAC ]; then
	clang++ --version
else
	g++ --version
fi

### Linking:
if [ -f $SOFILE ]; then
	$LDD $SOFILE
else
	if [ -f $BINFILE ]; then
		$LDD $BINFILE
	fi
fi

