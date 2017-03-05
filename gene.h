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

#include "utils.h"
#include "infos.h"

class Gene
{
	const Infos& infos;
	std::vector<std::set<VideoID>> cache_load_set;
	double score;

	static double calculate_score(const std::vector<std::set<VideoID>>& cache_load_set, const Infos& infos);
	
public:
	typedef std::shared_ptr<Gene> ptr;
	Gene(std::vector<std::set<VideoID>> cache_load_set, double score, const Infos& infos);
	Gene(std::vector<std::set<VideoID>> cache_load_set, const Infos& infos);
	static Gene::ptr generate_one(const Infos& infos);
	double get_score();
	void recalculate_score();
	Gene::ptr cross(const Gene& other);
	std::vector<std::set<VideoID>> get_cache_load_set();
	double loading_percentage();
	virtual ~Gene();
};

