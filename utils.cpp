#include "utils.h"
#include <iostream>
using namespace std;
static time_type start_time;
std::default_random_engine random_generator = std::default_random_engine(565765154);
decltype(&std::chrono::high_resolution_clock::now) now = std::chrono::high_resolution_clock::now;

void tic() {
	start_time = std::chrono::high_resolution_clock::now();
}

void toc(string event) {
	auto t = chrono::high_resolution_clock::now();
	auto dt = chrono::duration_cast<std::chrono::nanoseconds>(t - start_time).count();
	cout << event << ": " << dt << " nanosec" << endl;
	start_time = t;
}

long timedif(time_type t1, time_type t2) {
	return chrono::duration_cast<std::chrono::microseconds>(t1 - t2).count();
}

std::vector<size_t> python_range(size_t num) {
	std::vector<size_t> indices(num);
	std::iota(begin(indices), end(indices), 0);
	return indices;
}

std::string timestamp()
{
	long timestamp = chrono::duration_cast<chrono::seconds>(now().time_since_epoch()).count();
	return to_string(timestamp);
}
