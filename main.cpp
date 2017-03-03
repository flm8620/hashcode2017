#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <random> 
#include <memory>
#include "infos.h"
#include "utils.h"
#include "connectionstatus.h"

#define FOR(i,N) for(long i=0;i<N;i++)
using namespace std;

std::default_random_engine random_generator(123457);





long main() {
	string file = "trending_today.in";
	//string file = "me_at_the_zoo.in";
	//string file = "videos_worth_spreading.in";
	//string file = "kittens.in";

	Infos infos(file);
	time_type t0, t1;

	ConnectionStatus cs(infos);
	cs.method1_fill_cache();




	//cs.submission(file + "out");
	return 0;
}