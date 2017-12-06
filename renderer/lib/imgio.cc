// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: imgio.cc


#include <cstdlib>
#include <vector>
#define cimg_display 0

#define cimg_use_jpeg
#define cimg_use_png
#include "CImg.h"

#include "imgproc.hh"
#include "lib/strutils.hh"
#include "lib/utils.hh"

using namespace cimg_library;
using namespace std;

Matuc read_img(const char* fname) {
	if (! exists_file(fname))
		error_exit(ssprintf("File \"%s\" not exists!", fname));
	CImg<unsigned char> img(fname);
  int channel = img.spectrum();
	m_assert(channel == 3 || channel == 1 || channel == 4);
	Matuc mat(img.height(), img.width(), (channel == 1 ? 3: channel));
	m_assert(mat.rows() > 1 && mat.cols() > 1);

  REP(i, mat.rows())
    REP(j, mat.cols())
		  if (channel == 1)
				REP(k, 3)
					mat.at(i, j, k) = img(j, i, 0);
			else
	      REP(k, channel)
	        mat.at(i, j, k) = img(j, i, k);
	return mat;
}


void write_rgb(const char* fname, const Mat32f& mat) {
	m_assert(mat.channels() == 3);
	CImg<unsigned char> img(mat.cols(), mat.rows(), 1, 3);
	REP(i, mat.rows())
		REP(j, mat.cols()) {
			// use white background. Color::NO turns to 1
			img(j, i, 0) = (mat.at(i, j, 0) < 0 ? 1 : mat.at(i, j, 0)) * 255;
			img(j, i, 1) = (mat.at(i, j, 1) < 0 ? 1 : mat.at(i, j, 1)) * 255;
			img(j, i, 2) = (mat.at(i, j, 2) < 0 ? 1 : mat.at(i, j, 2)) * 255;
		}
	img.save(fname);
}

void write_rgb(const char* fname, const Matuc& mat) {
	m_assert(mat.channels() == 3);
	CImg<unsigned char> img(mat.cols(), mat.rows(), 1, 3);
	REP(i, mat.rows())
		REP(j, mat.cols()) {
			img(j, i, 0) = mat.at(i, j, 0);
			img(j, i, 1) = mat.at(i, j, 1);
			img(j, i, 2) = mat.at(i, j, 2);
		}
	img.save(fname);
}
