#!/bin/bash
# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

cd "${0%/*}"

if [ `uname` = 'Darwin' ]; then
	IS_MAC="1"
	LDD="otool -L"
else
	IS_MAC=""
	LDD="ldd"
fi
SOFILE=../House3D/objrender.so
BINFILE=./test-rectangle.bin

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
echo "DISPLAY:
$DISPLAY"
echo "HOUSE3D_FORCE_EGL:
$HOUSE3D_FORCE_EGL"
echo
PYCONFIG=${PYTHON_CONFIG:-python-config}

##### Flags:
set -x
pkg-config --cflags --libs glfw3

if [ ! "$IS_MAC" ]; then
	pkg-config --cflags --libs libglvnd
	pkg-config --cflags --libs egl

  echo "ldconfig:"
  ldconfig -p | grep EGL
fi

which pkg-config
which $PYCONFIG
$PYCONFIG --includes --ldflags
if [ $IS_MAC ]; then
  which find
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


set +x
if [ -f $BINFILE ]; then
  if [ ! "$IS_MAC" ]; then
    echo "Testing EGL backend ..."
    $BINFILE egl
  fi

  echo "Testing headless backend (GLX on Linux, CGL on mac)..."
  $BINFILE headless
fi
