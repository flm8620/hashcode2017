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
	//string file = "trending_today.in";
	//string file = "me_at_the_zoo.in";
	string file = "videos_worth_spreading.in";
	//string file = "kittens.in";

	Infos infos(file);
	time_type t0, t1;
	t0 = now();

	vector<shared_ptr<Gene>> genes(10, nullptr);
	for (auto& p : genes) {
		p = Gene::generate_one(random_generator, infos);
		cout << "gen score=" << p->get_score() << endl;
	}


	t1 = now();
	cout << "time lapse = " << timedif(t1, t0) / 1000000.0 << "secs" << endl;
	//cs.submission(file + "out");
	return 0;
}