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


long main() {
	//string file = "trending_today.in";
	//string file = "me_at_the_zoo.in";
	//string file = "videos_worth_spreading.in";
	string file = "kittens.in";
	cout << "Build infos..." << endl;
	Infos infos(file);
	cout << infos.num_cache << " caches, " << infos.num_user << " users, " << infos.num_video << " videos" << endl;
	time_type t0, t1;

	//cout << "Starting genetic algo..." << endl;
	//auto genes = genetic_algo(infos, 10, 10, 50, 1.0);

	//std::vector<std::vector<size_t>> orders;
	//read_file_order(orders, "order_file_72739_gen_26.order");
	cout << "Build ConnectionStatus..." << endl;
	ConnectionStatus cs(infos);
	cout << "Starting method..." << endl;
	t0 = now();
	//cs.method1_fill_cache(genes[0]->get_order(), 10, true);
	//cs.read_submission("me_at_the_zoo.in.out");
	cs.read_submission("kittens.in.out.best");
	cs.method5_following_touched_caches(10000, true);
	//cs.method1_fill_cache(std::vector<size_t>(), 10, true);
	t1 = now();
	cout << "score = " << cs.score << endl;
	cout << "time = " << timedif(t1, t0) / 1000000.0 << " secs" << endl;

	cs.submission(file + timestamp() + ".out");
	return 0;
}