#include "connectionstatus.h"
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

double ConnectionStatus::score_changes_if_add_video_to_cache(VideoID video_id, CacheID cache_id) {
	//time_type t0, t1, t2, t3, t4;
	double score_change = 0.0;
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
	return score_change * 1000.0 / infos.total_req_count;
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

void ConnectionStatus::add_video_to_cache(VideoID video_id, CacheID cache_id) {
	auto& cache_set = cache_load_set[cache_id];
	auto& video_existance = video_exist_in_cache[video_id];
	if (!cache_set.insert(video_id).second)
		throw "video already exist";
	if (!video_existance.insert(cache_id).second)
		throw "video already exist";
	cache_current_volume[cache_id] += infos.size_videos[video_id];
	//if ((cache_current_volume[cache_id] += infos.size_videos[video_id]) > infos.size_cache)
	//	throw "cache is full";


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
				if (old_cache_id != -1)
					cache_video_to_where[old_cache_id][video_id].erase(user_id);
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

void ConnectionStatus::fill_cache_by_best_videos(CacheID cache_id) {
	time_type t0, t1, t2, t3;
	const vector<VideoID>& video_list = infos.cache_may_have_video[cache_id];
	vector<double> video_score(video_list.size(), 0.0);
	//cout << "c=" << cache_id << " may have video = " << video_list.size() << endl;
	t0 = now();
	FOR(i, video_list.size()) {
		VideoID v = video_list[i];
		double score_change = score_changes_if_add_video_to_cache(v, cache_id);
		video_score[i] = score_change / infos.size_videos[v];
	}
	std::vector<size_t> indices = sort_index_large_to_small(video_score);
	long insert_count = 0;
	for (auto index : indices) {
		VideoID v_id = video_list[index];
		if (video_score[index] == 0) break;
		long rest_volume = infos.size_cache - cache_current_volume[cache_id];
		if (infos.size_videos[v_id] <= rest_volume) {
			add_video_to_cache(v_id, cache_id);
			insert_count += 1;
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

void ConnectionStatus::method1_fill_cache(const std::vector<size_t>& order) {
	time_type t0, t1, t2;
	std::vector<size_t> cache_order;
	if (order.empty()) {
		cache_order = python_range(infos.num_cache);
	}
	else {
		cache_order = order;
	}
	t0 = now();
	for(CacheID c : cache_order) {
		fill_cache_by_best_videos(c);
		//cout << "c=" << c << " t=" << timedif(t1, t0) / 1000 << "microsecs" << endl;
		//cout << "score = " << score << endl;
	}
	t1 = now();
	//cout << " time = " << timedif(t1, t0) / 1000.0 << " msecs" << endl;
	//cout << "score = " << score << endl;
	//submission("out.out");
	FOR(i, 3) {
		//std::vector<size_t> indices(infos.num_cache);
		//std::iota(begin(indices), end(indices), 0);
		//shuffle(indices.begin(), indices.end(), random_generator);
		t0 = now();
		for (auto c : cache_order) {
			empty_cache(c);
			fill_cache_by_best_videos(c);
		}
		t1 = now();
		//cout << "del time = " << timedif(t1, t0) / 1000.0 << " msecs" << endl;
		//cout << "score = " << score << endl;
		//submission("out.out");
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
			double move_score = score_changes_if_add_video_to_cache(v, c);
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
	int num_cache_from_file;
	f >> num_cache_from_file;
	if (num_cache_from_file != infos.num_cache) throw "error";
	FOR(i, infos.num_cache) {
		int video;
		f >> video;
		add_video_to_cache(video, i);
	}
	f.close();
}

Gene::ptr ConnectionStatus::generate_gene(std::default_random_engine & random_generator)
{
	init_all();
	auto indices = python_range(infos.num_cache);
	shuffle(indices.begin(), indices.end(), random_generator);
	for (auto c : indices) {
		fill_cache_by_best_videos(c);
	}
	return make_shared<Gene>(this->cache_load_set, this->score, infos);
}
