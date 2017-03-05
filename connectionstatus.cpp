#include "connectionstatus.h"
#include "knapsack.h"
#include <sstream>
using namespace std;
ConnectionStatus::ConnectionStatus(const Infos & infos) : infos(infos) {
	init_all();
}

void ConnectionStatus::init_all()
{
	user_video_from_where = infos.user_reqs;
	for (auto& m : user_video_from_where)
		for (auto& video_where : m)
			video_where.second = -1; //from server

	cache_load_set.clear();
	cache_load_set.resize(infos.num_cache);

	cache_video_to_where.clear();
	cache_video_to_where.resize(infos.num_cache);
	for (auto& it : cache_video_to_where)
		it.resize(infos.num_video);

	video_exist_in_cache.clear();
	video_exist_in_cache.resize(infos.num_video);

	cache_current_volume.clear();
	cache_current_volume.resize(infos.num_cache, 0);

	score = 0.0;
}

long long ConnectionStatus::score_changes_if_add_video_to_cache(VideoID video_id, CacheID cache_id) {
	//time_type t0, t1, t2, t3, t4;
	long long score_change = 0;
	for (UserID user_id : infos.cache_video_to_possible_user[cache_id][video_id]) {
		//t0 = now();
		auto it = user_video_from_where[user_id].find(video_id);
		//t1 = now();
		if (it != user_video_from_where[user_id].end()) {
			CacheID old_cache_id = it->second;
			//Delay delta = infos.user_cache_delay[user_id].at(old_cache_id) - infos.user_cache_delay[user_id].at(cache_id);
			Delay delta = infos.get_cache_user_delay(old_cache_id, user_id) - infos.get_cache_user_delay(cache_id, user_id);
			if (delta >= 0) {//added video is closer to user
				score_change += infos.user_reqs[user_id].at(video_id) * delta;
			}
		}
		//t2 = now();
		//cout << "t012=" << timedif(t1, t0) << " " << timedif(t2, t1) << endl;
	}
	return score_change;
}

double ConnectionStatus::score_changes_if_del_video_from_cache(VideoID video_id, CacheID cache_id)
{
	auto& cache_set = cache_load_set[cache_id];
	auto& video_existance = video_exist_in_cache[video_id];

	if (cache_set.find(video_id) == cache_set.end())
		throw "error";
	if (video_existance.find(cache_id) == video_existance.end())
		throw "error";
	if (cache_current_volume[cache_id] - infos.size_videos[video_id] < 0)
		throw "cache volume negative!";


	double score_change = 0.0;
	auto& user_set = cache_video_to_where[cache_id][video_id];
	for (UserID user_id : user_set) {
		const set<CacheID>& all_cache_connected_to_user = infos.user_to_cache[user_id];
		set<CacheID> possible_cache;
		set_intersection(all_cache_connected_to_user.begin(), all_cache_connected_to_user.end(),
			video_existance.begin(), video_existance.end(), inserter(possible_cache, possible_cache.begin()));
		CacheID dest_cache = -1;
		Delay min_time = infos.get_server_user_delay(user_id);
		for (CacheID c : possible_cache) {
			if (c == cache_id) continue;
			Delay time = infos.get_cache_user_delay(c, user_id);
			if (time < min_time) {
				dest_cache = c;
				min_time = time;
			}
		}

		score_change += infos.user_reqs[user_id].at(video_id) * (infos.get_cache_user_delay(cache_id, user_id) - min_time);
	}
	return score_change * 1000.0 / infos.total_req_count;
}

void ConnectionStatus::add_video_to_cache(VideoID video_id, CacheID cache_id, std::set<CacheID> *touched_caches) {
	auto& cache_set = cache_load_set[cache_id];
	auto& video_existance = video_exist_in_cache[video_id];
	if (!cache_set.insert(video_id).second)
		throw "video already exist";
	if (!video_existance.insert(cache_id).second)
		throw "video already exist";
	cache_current_volume[cache_id] += infos.size_videos[video_id];
	//if ((cache_current_volume[cache_id] += infos.size_videos[video_id]) > infos.size_cache)
	//	throw "cache is full";

	bool record_touched_caches = false;
	if (touched_caches)
		record_touched_caches = true;

	double score_change = 0.0;
	for (UserID user_id : infos.cache_video_to_possible_user[cache_id][video_id]) {
		auto it = user_video_from_where[user_id].find(video_id);
		if (it != user_video_from_where[user_id].end()) {
			CacheID old_cache_id = it->second;
			Delay delta = infos.get_cache_user_delay(old_cache_id, user_id) - infos.get_cache_user_delay(cache_id, user_id);
			if (delta >= 0) {//added video is closer to user
							 //add new connection
				if (!cache_video_to_where[cache_id][video_id].insert(user_id).second)
					throw "error";
				//break old connection
				if (old_cache_id != -1) {
					cache_video_to_where[old_cache_id][video_id].erase(user_id);
					if (record_touched_caches)
						(*touched_caches).insert(old_cache_id);
				}
				score_change += infos.user_reqs[user_id].at(video_id) * delta;
				user_video_from_where[user_id][video_id] = cache_id;
			}
		}
	}
	score += score_change * 1000.0 / infos.total_req_count;
}

void ConnectionStatus::del_video_from_cache(VideoID video_id, CacheID cache_id) {
	auto& cache_set = cache_load_set[cache_id];
	auto& video_existance = video_exist_in_cache[video_id];

	if (cache_set.erase(video_id) == 0)
		throw "error";
	if (video_existance.erase(cache_id) == 0)
		throw "error";
	if ((cache_current_volume[cache_id] -= infos.size_videos[video_id]) < 0)
		throw "cache volume negative!";


	double score_change = 0.0;
	auto& user_set = cache_video_to_where[cache_id][video_id];
	for (UserID user_id : user_set) {
		const set<CacheID>& all_cache_connected_to_user = infos.user_to_cache[user_id];
		set<CacheID> possible_cache;
		set_intersection(all_cache_connected_to_user.begin(), all_cache_connected_to_user.end(),
			video_existance.begin(), video_existance.end(), inserter(possible_cache, possible_cache.begin()));
		CacheID dest_cache = -1;
		Delay min_time = infos.get_server_user_delay(user_id);
		for (CacheID c : possible_cache) {
			Delay time = infos.get_cache_user_delay(c, user_id);
			if (time < min_time) {
				dest_cache = c;
				min_time = time;
			}
		}
		if (dest_cache != -1)
			if (!cache_video_to_where[dest_cache][video_id].insert(user_id).second)
				throw "error";

		score_change += infos.user_reqs[user_id].at(video_id) * (
			infos.get_cache_user_delay(cache_id, user_id) - min_time
			);
		user_video_from_where[user_id][video_id] = dest_cache;
	}
	cache_video_to_where[cache_id][video_id].clear();
	score += score_change * 1000.0 / infos.total_req_count;
}

void ConnectionStatus::fill_cache_by_best_videos(CacheID cache_id, int size_limit, std::set<CacheID> *touched_caches) {
	if (size_limit == 0)size_limit = infos.size_cache;
	time_type t0, t1, t2, t3;
	const vector<VideoID>& video_list = infos.cache_may_have_video[cache_id];
	vector<double> video_value(video_list.size(), 0.0);
	//cout << "c=" << cache_id << " may have video = " << video_list.size() << endl;
	t0 = now();
	FOR(i, video_list.size()) {
		VideoID v = video_list[i];
		double score_change = score_changes_if_add_video_to_cache(v, cache_id) * 1000.0 / infos.total_req_count;
		video_value[i] = score_change;
	}
	std::vector<size_t> indices = sort_index_large_to_small(video_value);


	long insert_count = 0;
	for (int index : indices) {
		VideoID v_id = video_list[index];
		if (video_value[index] == 0.0) break;
		long rest_volume = size_limit - cache_current_volume[cache_id];
		if (infos.size_videos[v_id] <= rest_volume) {
			add_video_to_cache(v_id, cache_id, touched_caches);
			insert_count += 1;
		}
	}
	//cout << "inserted video:" << insert_count << endl;
	//cout << "rest_volume: " << infos.size_cache - cache_current_volume[cache_id] << endl;
}

void ConnectionStatus::fill_cache_by_best_videos_knapsack(CacheID cache_id, int size_limit, std::set<CacheID>* touched_caches)
{
	if (size_limit == 0)size_limit = infos.size_cache;
	time_type t0, t1, t2, t3;
	const vector<VideoID>& video_list = infos.cache_may_have_video[cache_id];
	vector<long long> video_value(video_list.size());
	//cout << "c=" << cache_id << " may have video = " << video_list.size() << endl;
	t0 = now();
	FOR(i, video_list.size()) {
		VideoID v = video_list[i];
		long long score_change = score_changes_if_add_video_to_cache(v, cache_id);
		video_value[i] = score_change;
	}
	//std::vector<size_t> indices = sort_index_large_to_small(video_value);

	vector<int> video_weight(video_list.size());
	FOR(i, video_list.size()) {
		video_weight[i] = infos.size_videos[video_list[i]];
	}
	set<int> solutions;
	knapsack(video_list.size(), infos.size_cache, video_weight, video_value, solutions);

	long insert_count = 0;
	for (int index : solutions) {
		VideoID v_id = video_list[index];
		long rest_volume = size_limit - cache_current_volume[cache_id];
		if (infos.size_videos[v_id] <= rest_volume) {
			add_video_to_cache(v_id, cache_id, touched_caches);
			insert_count += 1;
		}
		else {
			throw "error";
		}
	}
	//cout << "inserted video:" << insert_count << endl;
	//cout << "rest_volume: " << infos.size_cache - cache_current_volume[cache_id] << endl;
}

double ConnectionStatus::calculate_score() {
	long long save_time = 0;
	long total_req = 0;
	for (auto r : infos.reqs) {
		CacheID c = user_video_from_where[r.user][r.video];
		save_time += r.number * (infos.get_cache_user_delay(-1, r.user) - infos.get_cache_user_delay(c, r.user));
		total_req += r.number;
	}
	return 1000.0 * save_time / total_req;
}

void ConnectionStatus::empty_cache(CacheID cache_id) {
	while (!cache_load_set[cache_id].empty()) {
		VideoID v = *(cache_load_set[cache_id].begin());
		del_video_from_cache(v, cache_id);
	}
}

void ConnectionStatus::method1_fill_cache(const std::vector<size_t>& order, int refill_times, bool verbose) {
	time_type t0, t1, t2;
	std::vector<size_t> cache_order;
	if (order.empty()) {
		cache_order = python_range(infos.num_cache);
	}
	else {
		cache_order = order;
	}
	//submission("out.out");
	FOR(i, refill_times + 1) {
		t0 = now();
		for (auto c : cache_order) {
			empty_cache(c);
			fill_cache_by_best_videos_knapsack(c);
		}
		t1 = now();
		if (verbose)
			cout << "time = " << timedif(t1, t0) / 1000.0 << " msecs" << endl << "score = " << score << endl;
		//submission("out.out");
	}
}

void ConnectionStatus::method5_following_touched_caches(int follow_times, bool verbose)
{
	time_type t0, t1, t2;

	set<CacheID> touched_caches;
	CacheID next = -1;
	CacheID start;
	uniform_int_distribution<int> distri(0, infos.num_cache - 1);
	uniform_real_distribution<double> proba(0, 1);
	cout << "start following..." << endl;
	FOR(i, follow_times) {
		if (i != 0 && i % 500 == 0) {
			cout << "submission" << endl;
			submission(string("follow_") + timestamp() + " " + to_string(i) + ".out");
		}
		if (next == -1 || proba(random_generator) < 0.01) {
			start = distri(random_generator);
			cout << "######## new start ##########: \t" << start << endl;
		}
		else {
			start = next;
		}
		cout << "filling\t\t" << start << endl;
		empty_cache(start);
		touched_caches.clear();
		fill_cache_by_best_videos_knapsack(start, 0, &touched_caches);

		//random element
		if (!touched_caches.empty()) {
			cout << touched_caches.size() << "caches touched" << endl;
			uniform_int_distribution<int> distri2(0, touched_caches.size() - 1);
			int rand_pos = distri2(random_generator);
			auto it = touched_caches.begin();
			advance(it, rand_pos);
			next = *it;
		}
		else {
			next = -1;
		}
		cout << "score = \t\t" << score << "\t" << i << endl;
	}

}

void ConnectionStatus::update_best_cache(vector<vector<bool>>& video_inserted_to_cache, vector<CacheID>& best_cache_to_insert_for_video, vector<double>& score_for_best_cache_to_insert_for_video, vector<set<VideoID>>& videos_choose_cache, VideoID v) {
	CacheID best_cache_id = -1;
	double best_score = -1.0;
	if (best_cache_to_insert_for_video[v] >= 0) {
		if (videos_choose_cache[best_cache_to_insert_for_video[v]].erase(v) == 0) throw "not there";
	}
	FOR(c, infos.num_cache) {
		if (!video_inserted_to_cache[v][c] && cache_current_volume[c] + infos.size_videos[v] <= infos.size_cache) {
			double move_score = score_changes_if_add_video_to_cache(v, c) * 1000.0 / infos.total_req_count;
			if (move_score > 0 && move_score > best_score) {
				best_score = move_score; best_cache_id = c;
			}
		}
	}
	best_cache_to_insert_for_video[v] = best_cache_id;
	score_for_best_cache_to_insert_for_video[v] = best_score;
	if (best_cache_id != -1)
		if (!videos_choose_cache[best_cache_id].insert(v).second) throw "impossible";
}

void ConnectionStatus::method4_fill_cache_gradually(const std::vector<size_t>& order, int refill_times, int repeat_time, bool verbose)
{
	time_type t0, t1, t2;
	std::vector<size_t> cache_order;
	if (order.empty()) {
		cache_order = python_range(infos.num_cache);
	}
	else {
		cache_order = order;
	}
	FOR(i, refill_times + 1) {
		FOR(r, repeat_time) {
			t0 = now();
			int size_limit = infos.size_cache * (double(i + 1) / (refill_times + 1));
			for (auto c : cache_order) {
				empty_cache(c);
				fill_cache_by_best_videos_knapsack(c, size_limit);
			}
			t1 = now();
			if (verbose) {
				cout << "time = " << timedif(t1, t0) / 1000.0 << " msecs" << endl;
				cout << "score = " << score << endl;
				cout << "size limit = " << size_limit << endl;

			}
			//submission("out.out");
		}
	}
}

inline void ConnectionStatus::method2_take_best_move() {
	CacheID best_cache_id;
	VideoID best_video_id;
	double best_score;
	long long counter = 0;
	vector<vector<bool>> video_inserted_to_cache(infos.num_video, vector<bool>(infos.num_cache, false));
	vector<CacheID> best_cache_to_insert_for_video(infos.num_video, -1);
	vector<double> score_for_best_cache_to_insert_for_video(infos.num_video, -1.0);
	vector<set<VideoID>> videos_choose_cache(infos.num_cache);

	FOR(v, infos.num_video) {
		update_best_cache(video_inserted_to_cache, best_cache_to_insert_for_video, score_for_best_cache_to_insert_for_video, videos_choose_cache, v);
	}

	auto sort_index_fun = [&](VideoID v1, VideoID v2)->bool {
		return score_for_best_cache_to_insert_for_video[v1] > score_for_best_cache_to_insert_for_video[v2];
	};
	vector<size_t> video_index(infos.num_video);
	iota(begin(video_index), end(video_index), 0);


	while (1) {
		time_type t0, t1, t2, t3;
		t0 = now();
		best_video_id = -1;
		best_cache_id = -1;

		double best_score = 0.0;
		sort(begin(video_index), end(video_index), sort_index_fun);
		t1 = now();
		best_video_id = video_index[0];
		best_cache_id = best_cache_to_insert_for_video[best_video_id];
		best_score = score_for_best_cache_to_insert_for_video[best_video_id];
		if (best_cache_id == -1) break;

		add_video_to_cache(best_video_id, best_cache_id);
		video_inserted_to_cache[best_video_id][best_cache_id] = true;
		t2 = now();
		update_best_cache(video_inserted_to_cache, best_cache_to_insert_for_video, score_for_best_cache_to_insert_for_video, videos_choose_cache, best_video_id);
		vector<VideoID> vec_videos_choose_cache(videos_choose_cache[best_cache_id].begin(), videos_choose_cache[best_cache_id].end());
		cout << vec_videos_choose_cache.size() << " videos to update" << endl;
		for (VideoID video_id : vec_videos_choose_cache) {
			update_best_cache(video_inserted_to_cache, best_cache_to_insert_for_video, score_for_best_cache_to_insert_for_video, videos_choose_cache, video_id);
		}
		t3 = now();
		cout << counter << ": video " << best_video_id << " into cache " << best_cache_id << " score " << best_score << endl;
		cout << "score " << score << endl;
		cout << "time: " << timedif(t1, t0) << " " << timedif(t2, t1) << " " << timedif(t3, t2) << endl;
		counter++;
	}
}

void ConnectionStatus::method3_fill_all_then_delete()
{
	time_type t0, t1, t2, t3;
	struct DeleteDecision {
		CacheID c;
		VideoID v;
		double score;
	};
	vector<DeleteDecision> decisions;

	long total_left_count = 0;
	FOR(v, infos.num_video) {
		if (v % 100 == 0)cout << "video " << v << endl;
		t0 = now();
		FOR(c, infos.num_cache) {
			add_video_to_cache(v, c);
		}
		t1 = now();
		//cout << "add all time = " << timedif(t1, t0) / 1000.0 << " msecs" << endl;
		//cout << "deleting unused videos" << endl;
		long left_count = infos.num_cache;
		t0 = now();
		FOR(c, infos.num_cache) {
			if (score_changes_if_del_video_from_cache(v, c) == 0.0) {
				del_video_from_cache(v, c);
				left_count--;
			}
		}

		for (auto& c : video_exist_in_cache[v]) {
			double s = score_changes_if_del_video_from_cache(v, c) / infos.size_videos[v];
			decisions.push_back(DeleteDecision{ c,v,s });
		}
		t1 = now();
		//cout << "delete time = " << timedif(t1, t0) / 1000.0 << " msecs" << endl;
		//cout << left_count << " videos left" << endl;
		total_left_count += left_count;
	}
	cout << "total video left = " << total_left_count << " " << decisions.size() << endl;
	this->submission("temp.out");
	auto sort_fun = [&](const DeleteDecision& a, const DeleteDecision& b)->bool {
		return a.score < b.score;
	};
	t0 = now();
	sort(decisions.begin(), decisions.end(), sort_fun);
	t1 = now();
	cout << "sort time = " << timedif(t1, t0) / 1000.0 << "msecs" << endl;

	for (const DeleteDecision& d : decisions) {
		VideoID v = d.v;
		CacheID c = d.c;
		if (cache_current_volume[c] > infos.size_cache) {
			del_video_from_cache(v, c);
			cout << score << endl;
		}
	}
}

void ConnectionStatus::submission(string outfile) {
	ofstream f(outfile);
	f << infos.num_cache << endl;
	FOR(i, infos.num_cache) {
		f << i << " ";
		for (VideoID v : cache_load_set[i]) f << v << " ";
		f << endl;
	}
	f.close();
}

void ConnectionStatus::read_submission(string file) {
	ifstream f(file);
	cout << "reading file" << endl;
	std::string line;
	std::getline(f, line);
	std::istringstream iss(line);
	int num_cache_from_file;
	iss >> num_cache_from_file;
	if (num_cache_from_file != infos.num_cache) throw "error";
	FOR(i, infos.num_cache) {
		std::getline(f, line);
		std::istringstream ss(line);
		int cache_num;
		ss >> cache_num;
		if (cache_num != i) throw "error";
		do {
			int video;
			ss >> video;
			if (!ss.good()) break;
			add_video_to_cache(video, i);
		} while (1);

	}
	f.close();
}

Gene::ptr ConnectionStatus::generate_gene()
{
	init_all();
	auto indices = python_range(infos.num_cache);
	shuffle(indices.begin(), indices.end(), random_generator);
	for (auto c : indices) {
		fill_cache_by_best_videos_knapsack(c);
	}
	return make_shared<Gene>(this->cache_load_set, this->score, infos);
}
