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
#include "ordergene.h"
#define FOR(i,N) for(long i=0;i<N;i++)
using namespace std;

std::default_random_engine random_generator(224469);





long main() {
	//string file = "trending_today.in";
	string file = "me_at_the_zoo.in";
	//string file = "videos_worth_spreading.in";
	//string file = "kittens.in";
	cout << "Build infos..." << endl;
	Infos infos(file);
	cout << infos.num_cache << " caches, " << infos.num_user << " users, " << infos.num_video << " videos" << endl;
	time_type t0, t1;
	cout << "Build ConnectionStatus..." << endl;

	auto genes = genetic_algo(infos, random_generator, 10, 20, 100, 0.3);

	ConnectionStatus cs(infos);
	cout << "Starting method..." << endl;
	t0 = now();
	cs.method1_fill_cache(genes[0]->get_order());
	t1 = now();
	cout << "time = " << timedif(t1, t0) / 1000000.0 << " secs" << endl;




	cs.submission(file + "out");
	return 0;
}