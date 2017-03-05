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
	OrderGene(const OrderGene& other);
	OrderGene::ptr clone();
	OrderGene::ptr crossover(const OrderGene& other, std::default_random_engine & random_generator) const;
	int mutate(std::default_random_engine & random_generator, double continue_proba = 0.9);
	int mutate2(std::default_random_engine & random_generator, double continue_proba = 0.5);

	double get_score()const { return score; }
	int get_id()const { return id; }
	std::vector<size_t> get_order() { return order; }
	const std::vector<size_t>& get_order_ref() { return order; }
	~OrderGene();
};

std::vector<OrderGene::ptr> genetic_algo(const Infos& infos, std::default_random_engine & random_generator, int num_genes, int num_growth, int num_generations, double mutate_proba, std::string initial_file = std::string(""));

void write_file(const std::vector<OrderGene::ptr>& genes, std::string filename);
void read_file(std::vector<OrderGene::ptr>& genes, std::string filename, const Infos& infos);
void read_file_order(std::vector<std::vector<size_t>>& orders, std::string filename);