#pragma once
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
#include "gene.h"

class ConnectionStatus {

	const Infos& infos;

	std::vector<std::map<VideoID, CacheID>> user_video_from_where;
	std::vector<std::set<VideoID>> cache_load_set;
	std::vector<std::vector<std::set<UserID>>> cache_video_to_where;
	std::vector<std::set<CacheID>> video_exist_in_cache;
	std::vector<int> cache_current_volume;


	long long score_changes_if_add_video_to_cache(VideoID video_id, CacheID cache_id);
	double score_changes_if_del_video_from_cache(VideoID video_id, CacheID cache_id);
	void update_best_cache(
		std::vector<std::vector<bool>>& video_inserted_to_cache,
		std::vector<CacheID>& best_cache_to_insert_for_video,
		std::vector<double>& score_for_best_cache_to_insert_for_video,
		std::vector<std::set<VideoID>>& videos_choose_cache,
		VideoID v);
public:

	double score;
	ConnectionStatus(const Infos& infos);

	void init_all();

	void add_video_to_cache(VideoID video_id, CacheID cache_id, std::set<CacheID> *touched_caches = nullptr);

	void del_video_from_cache(VideoID video_id, CacheID cache_id);

	void fill_cache_by_best_videos(CacheID cache_id, int size_limit = 0, std::set<CacheID> *touched_caches = nullptr);
	void fill_cache_by_best_videos_knapsack(CacheID cache_id, int size_limit = 0, std::set<CacheID> *touched_caches = nullptr);

	double calculate_score();

	void empty_cache(CacheID cache_id);

	void method1_fill_cache(const std::vector<size_t>& order = std::vector<size_t>(), int refill_times = 0, bool verbose = false);
	void method5_following_touched_caches(int follow_times = 10000, bool verbose = false);
	void method4_fill_cache_gradually(const std::vector<size_t>& order = std::vector<size_t>(), int refill_times = 0, int repeat_time = 1, bool verbose = false);
	void method2_take_best_move();
	void method3_fill_all_then_delete();
	void stupid_method3();
	void submission(std::string outfile);
	void read_submission(std::string file);
	Gene::ptr generate_gene();

};
