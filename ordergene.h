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
class OrderGene
{
	std::vector<size_t> order;
	const Infos& infos;
	double score;
	void calculate_score();
	const int id;
	static int id_count;
public:
	typedef std::shared_ptr<OrderGene> ptr;
	OrderGene(const Infos& infos, std::default_random_engine& random_generator);
	OrderGene(const Infos& infos, const std::vector<size_t>& order);
	OrderGene::ptr crossover(const OrderGene& other, std::default_random_engine & random_generator) const;
	void mutate(std::default_random_engine & random_generator, int num_swap);
	double get_score()const { return score; }
	int get_id()const { return id; }
	std::vector<size_t> get_order() { return order; }
	~OrderGene();
};

std::vector<OrderGene::ptr> genetic_algo(const Infos& infos, std::default_random_engine & random_generator, int num_genes, int num_growth, int num_generations, double mutate_proba);