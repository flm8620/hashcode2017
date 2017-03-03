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

#include "utils.h"

typedef long Delay;
typedef long VideoID;
typedef long CacheID;
typedef long UserID;

struct Request {
	VideoID video;
	UserID user;
	long number;
};

struct Infos
{
	long num_video, num_user, num_req, num_cache, size_cache, total_req_count;

	std::vector<std::map<CacheID, Delay>> user_cache_delay; // cache -1 means server
	std::vector<int> size_videos;
	std::vector<Request> reqs;
	std::vector<std::set<UserID>> cache_to_user;
	std::vector<std::map<VideoID, int>> user_reqs;
	std::vector<std::vector<VideoID>> cache_may_have_video;
	std::vector<std::vector<std::set<UserID>>> cache_video_to_possible_user;

	Infos(std::string file);
	~Infos();

	inline Delay get_cache_user_delay(CacheID c, UserID u)const {
		return user_cache_delay[u].at(c);
	}

	void read_file(std::string file);
	
};

