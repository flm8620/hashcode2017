#pragma once
#include <chrono>
#include <string>
#include <algorithm>
#include <vector>
#include <numeric>
#define FOR(i,N) for(long i=0;i<N;i++)
typedef decltype(std::chrono::high_resolution_clock::now()) time_type;
extern decltype(&std::chrono::high_resolution_clock::now) now;

void tic();
void toc(std::string event);
long timedif(time_type t1, time_type t2);

std::vector<size_t> python_range(size_t num);

template <typename T>
std::vector<std::size_t> sort_index_large_to_small(std::vector<T> const& values) {
	std::vector<std::size_t> indices(values.size());
	std::iota(begin(indices), end(indices), 0);
	std::sort(begin(indices), end(indices),
		[&](size_t a, size_t b) { return values[a] > values[b]; }
	);
	return indices;
}