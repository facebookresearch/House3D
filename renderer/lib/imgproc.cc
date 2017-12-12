// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: imgproc.cc


#include "imgproc.hh"

#include <cmath>
#include <vector>

#include "utils.hh"
#include "debugutils.hh"
#include "timer.hh"

using namespace std;

namespace {

void resize_bilinear(const Mat32f &src, Mat32f &dst) {
	vector<int> tabsx(dst.rows());
	vector<int> tabsy(dst.cols());
	vector<float> tabrx(dst.rows());
	vector<float> tabry(dst.cols());

	const float fx = (float)(dst.rows()) / src.rows();
	const float fy = (float)(dst.cols()) / src.cols();
	const float ifx = 1.f / fx;
	const float ify = 1.f / fy;
	for (int dx = 0; dx < dst.rows(); ++dx) {
		float rx = (dx+0.5f) * ifx - 0.5f;
		int sx = floor(rx);
		rx -= sx;
		if (sx < 0) {
			sx = rx = 0;
		} else if (sx + 1 >= src.rows()) {
			sx = src.rows() - 2;
			rx = 1;
		}
		tabsx[dx] = sx;
		tabrx[dx] = rx;
	}
	for (int dy = 0; dy < dst.cols(); ++dy) {
		float ry = (dy+0.5f) * ify - 0.5f;
		int sy = floor(ry);
		ry -= sy;
		if (sy < 0) {
			sy = ry = 0;
			ry = 0;
		} else if (sy + 1 >= src.cols()) {
			sy = src.cols() - 2;
			ry = 1;
		}
		tabsy[dy] = sy;
		tabry[dy] = ry;
	}

	const int ch = src.channels();
	for (int dx = 0; dx < dst.rows(); ++dx) {
		const float *p0 = src.ptr(tabsx[dx]+0);
		const float *p1 = src.ptr(tabsx[dx]+1);
		float *pdst = dst.ptr(dx);
		float rx = tabrx[dx], irx = 1.0f - rx;
		for (int dy = 0; dy < dst.cols(); ++dy) {
			float *pcdst = pdst + dy*ch;
			const float *pc00 = p0 + (tabsy[dy]+0)*ch;
			const float *pc01 = p0 + (tabsy[dy]+1)*ch;
			const float *pc10 = p1 + (tabsy[dy]+0)*ch;
			const float *pc11 = p1 + (tabsy[dy]+1)*ch;
			float ry = tabry[dy], iry = 1.0f - ry;
			for (int c = 0; c < ch; ++c) {
				float res = rx * (pc11[c]*ry + pc10[c]*iry)
					+ irx * (pc01[c]*ry + pc00[c]*iry);
				pcdst[c] = res;
			}
		}
	}
}
}	// namespace


Mat32f rgb2grey(const Mat32f& mat) {
	m_assert(mat.channels() == 3);
	Mat32f ret(mat.height(), mat.width(), 1);
	const float* src = mat.ptr();
	float* dst = ret.ptr();
	int n = mat.pixels();
	int idx = 0;
	for (int i = 0; i < n; ++i) {
		dst[i] = (src[idx] + src[idx+1] + src[idx+2]) / 3.f;
		idx += 3;
	}
	return ret;
}

template <>
void resize<float>(const Mat32f &src, Mat32f &dst) {
	m_assert(src.rows() > 1 && src.cols() > 1);
	m_assert(dst.rows() > 1 && dst.cols() > 1);
	m_assert(src.channels() == dst.channels());
	m_assert(src.channels() == 1 || src.channels() == 3);
	return resize_bilinear(src, dst);
}

Matuc cvt_f2uc(const Mat32f& mat) {
	m_assert(mat.channels() == 3);
	Matuc ret(mat.rows(), mat.cols(), 3);
	auto ps = mat.ptr();
	auto pt = ret.ptr();
	int n = mat.pixels() * 3;
	REP(i, n)
		*(pt++) = *(ps++) * 255.0;
	return ret;
}

void vflip(Matuc& mat) {
  int len = mat.cols() * mat.channels() * sizeof(Matuc::value_type);
  char* buf = new char[len];
  int H = mat.rows();
  for (int h = 0; h < H; ++h) {
    int hh = H - 1 - h;
    if (h >= hh) break;
    auto ptr1 = mat.ptr(h),
         ptr2 = mat.ptr(hh);
    memcpy(buf, ptr1, len);
    memcpy(ptr1, ptr2, len);
    memcpy(ptr2, buf, len);
  }
  delete[] buf;
}

Matuc hconcat(std::vector<Matuc>& srcs) {
  int rows = srcs[0].rows();
  int channels = srcs[0].channels();
  int cols = 0;
  for (Matuc& cur : srcs) {
    m_assert(cur.rows() == rows);
    m_assert(cur.channels() == channels);
    cols += cur.cols();
  }

  Matuc buf(rows, cols, channels);
  int offset = 0;
  for (Matuc& cur : srcs) {
    int len = cur.cols() * channels;
    for (int r = 0; r < rows; r++) {
      auto out = buf.ptr(r, offset);
      auto in = cur.ptr(r);
      memcpy(out, in, len);
    }
    offset += cur.cols();
  }

  return buf;
}
