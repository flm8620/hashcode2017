#include "infos.h"

using std::vector;
using std::set;
using std::ifstream;
using std::string;


Infos::Infos(string file)
{
	read_file(file);
	cache_to_user.resize(num_cache);
	FOR(user_id, num_user)
		for (auto& it : user_cache_delay[user_id]) {
			CacheID c = it.first; Delay time = it.second;
			if (c != -1) cache_to_user[c].insert(user_id);
		}

	user_reqs.resize(num_user);
	for (auto& r : reqs)
		user_reqs[r.user][r.video] += r.number;

	cache_may_have_video.resize(num_cache);
	vector<set<VideoID>> cache_may_have_video_set_version;
	cache_may_have_video_set_version.resize(num_cache);
	FOR(cache_id, num_cache)
		for (UserID user_id : cache_to_user[cache_id])
			for (auto key_value : user_reqs[user_id]) {
				VideoID video_id = key_value.first;
				cache_may_have_video_set_version[cache_id].insert(video_id);
			}
	FOR(cache_id, num_cache) {
		cache_may_have_video[cache_id].assign(cache_may_have_video_set_version[cache_id].begin(), cache_may_have_video_set_version[cache_id].end());
	}
	vector<set<UserID>> temp(num_video);
	cache_video_to_possible_user.resize(num_cache, temp);

	FOR(cache_id, num_cache)
		for (UserID user_id : cache_to_user[cache_id])
			for (auto& video_delay : user_reqs[user_id])
				cache_video_to_possible_user[cache_id][video_delay.first].insert(user_id);
}

Infos::~Infos()
{
}


void Infos::read_file(string file) {
	ifstream f(file);
	if (!f.good()) throw "file error";
	f >> num_video >> num_user >> num_req >> num_cache >> size_cache;
	size_videos.resize(num_video);
	FOR(i, num_video)
		f >> size_videos[i];

	user_cache_delay.resize(num_user);
	FOR(i, num_user) {
		Delay server_delay;        f >> server_delay;
		long cache_count;           f >> cache_count;
		auto& dict = user_cache_delay[i];
		dict[-1] = server_delay;
		for (long c = 0; c < cache_count; c++) {
			CacheID cache_id;
			f >> cache_id;
			Delay delay;
			f >> delay;
			dict[cache_id] = delay;
		}
	}
	reqs.resize(num_req);
	FOR(i, num_req) {
		f >> reqs[i].video >> reqs[i].user >> reqs[i].number;
		total_req_count += reqs[i].number;
	}
	f.close();
}