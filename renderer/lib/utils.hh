// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
// File: utils.hh
// Date: Fri Jun 23 17:05:07 2017 -0700



#pragma once

#include <cstdlib>
#include <memory>
#include <sys/stat.h>
#include <type_traits>

const double EPS = 1e-6;
const double GEO_EPS_SQR = 1e-14;
const double GEO_EPS = 1e-7;
inline float sqr(float x) { return x * x; }

#define between(a, b, c) ((a >= b) && (a <= c - 1))
#define REP(x, y) for (typename std::remove_cv<typename std::remove_reference<decltype(y)>::type>::type x = 0; x < (y); x ++)
#define REPL(x, y, z) for (typename std::remove_cv<typename std::remove_reference<decltype(y)>::type>::type x = y; x < (z); x ++)
#define REPD(x, y, z) for (typename std::remove_cv<typename std::remove_reference<decltype(y)>::type>::type x = y; x >= (z); x --)

template<typename T>
inline bool update_min(T &dest, const T &val) {
	if (val < dest) {
		dest = val; return true;
	}
	return false;
}

template<typename T>
inline bool update_max(T &dest, const T &val) {
	if (dest < val) {
		dest = val; return true;
	}
	return false;
}

template <typename T>
inline void free_2d(T** ptr, int w) {
	if (ptr != nullptr)
		for (int i = 0; i < w; i ++)
			delete[] ptr[i];
	delete[] ptr;
}

template <typename T>
std::shared_ptr<T> create_auto_buf(size_t len, bool init_zero = false) {
	std::shared_ptr<T> ret(new T[len], std::default_delete<T[]>());
	if (init_zero)
		memset(ret.get(), 0, sizeof(T) * len);
	return ret;
}


inline bool exists_file(const char* name) {
	struct stat buffer;
	return stat(name, &buffer) == 0;
}
