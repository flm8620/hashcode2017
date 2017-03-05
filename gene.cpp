#include "gene.h"
#include "connectionstatus.h"
using namespace std;


double Gene::calculate_score(const std::vector<std::set<VideoID>>& cache_load_set, const Infos& infos)
{
	double gene_score = 0.0;
	FOR(u, infos.num_user) {
		const std::map<VideoID, long>& requests = infos.user_reqs[u];
		size_t video_count = requests.size();
		Delay server_delay = infos.user_cache_delay[u].at(-1);
		vector<long> nearest_cache(video_count, -1);
		vector<long> shortest_delay(video_count, server_delay);

		const std::map<CacheID, Delay>& connected_caches = infos.user_cache_delay[u];

		for (const auto& cache_delay : connected_caches) {
			CacheID c = cache_delay.first;
			if (c == -1) continue; //server
			Delay delay = cache_delay.second;
			const set<VideoID>& loads = cache_load_set[c];

			size_t i = 0;
			for (const auto& video_num : requests) {
				VideoID v = video_num.first;
				long demands = video_num.second;
				if (loads.find(v) != loads.end()) {
					if (delay < shortest_delay[i]) {
						nearest_cache[i] = c;
						shortest_delay[i] = delay;
					}
				}
				i++;
			}
		}

		size_t i = 0;
		for (const auto& video_num : requests) {
			long demands = video_num.second;
			gene_score += demands * (server_delay - shortest_delay[i]);
			i++;
		}


	}
	return gene_score * 1000.0 / infos.total_req_count;
}

Gene::Gene(std::vector<std::set<VideoID>> cache_load_set, double score, const Infos& infos) :cache_load_set(cache_load_set), score(score), infos(infos) {

}

Gene::Gene(std::vector<std::set<VideoID>> cache_load_set, const Infos & infos) : cache_load_set(cache_load_set), infos(infos) {
	score = calculate_score(cache_load_set, infos);
}

Gene::ptr Gene::generate_one(const Infos& infos)
{
	ConnectionStatus cs(infos);
	return cs.generate_gene();
}

double Gene::get_score()
{
	return score;
}

void Gene::recalculate_score()
{
	score = calculate_score(cache_load_set, infos);
}

Gene::ptr Gene::cross(const Gene & other)
{
	std::vector<std::set<VideoID>> new_cache_load_set;
	new_cache_load_set.resize(infos.num_cache);
	FOR(c, infos.num_cache) {
		set<VideoID> loads = this->cache_load_set[c];
		loads.insert(other.cache_load_set[c].begin(), other.cache_load_set[c].end());
		vector<VideoID> loads_vec(loads.begin(), loads.end());
		shuffle(begin(loads_vec), end(loads_vec), random_generator);

		int weight = 0;
		for (VideoID v : loads_vec) {
			if (weight + infos.size_videos[v] <= infos.size_cache) {
				weight += infos.size_videos[v];
				new_cache_load_set[c].insert(v);
			}
		}
	}
	return make_shared<Gene>(new_cache_load_set, infos);
}

std::vector<std::set<VideoID>> Gene::get_cache_load_set()
{
	return cache_load_set;
}

double Gene::loading_percentage()
{
	int sum = 0;
	for (auto& s : cache_load_set) {
		for (VideoID v : s) {
			sum += infos.size_videos[v];
		}
	}
	double p = double(sum) / cache_load_set.size();
	p /= infos.size_cache;
	return p;
}

Gene::~Gene()
{
}
