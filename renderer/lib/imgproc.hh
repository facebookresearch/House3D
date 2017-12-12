// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: imgproc.hh
//Date: Wed Sep 13 17:14:58 2017 -0700


#pragma once
#include <list>
#include <vector>
#include "mat.h"
#include "utils.hh"

// return a image of hxwx[1,3,4]
Matuc read_img(const char* fname);
void write_rgb(const char* fname, const Mat32f& mat);

void write_rgb(const char* fname, const Matuc& mat);

Mat32f rgb2grey(const Mat32f& mat);

template <typename T>
void fill(Mat<T>& mat, T c) {
	T* ptr = mat.ptr();
	int n = mat.elements();
	REP(i, n) {
		*ptr = c;
		ptr ++;
	}
}

template <typename T>
void resize(const Mat<T> &src, Mat<T> &dst);

Matuc cvt_f2uc(const Mat32f& mat);

// in-place vertical flip
void vflip(Matuc& mat);

Matuc hconcat(std::vector<Matuc>& srcs);
