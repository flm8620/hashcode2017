#include "ordergene.h"

#include "connectionstatus.h"
using namespace std;

int OrderGene::id_count = 0;
void OrderGene::calculate_score()
{
	ConnectionStatus cs(infos);
	cs.method1_fill_cache(order);
	score = cs.score;
}

OrderGene::OrderGene(const Infos& infos, std::default_random_engine & random_generator) : infos(infos), id(id_count++)
{
	order = python_range(infos.num_cache);
	shuffle(begin(order), end(order), random_generator);
	calculate_score();
}

OrderGene::OrderGene(const Infos & infos, const std::vector<size_t>& order) : infos(infos), id(id_count++)
{
	this->order = order;
	calculate_score();
}

OrderGene::ptr OrderGene::crossover(const OrderGene & other, std::default_random_engine & random_generator) const
{
	std::vector<size_t> new_order(infos.num_cache);
	uniform_int_distribution<int> distri(0, infos.num_cache);
	int a = distri(random_generator);
	int b = distri(random_generator);
	if (a > b) swap(a, b);
	//cout << "a,b=" << a << "," << b << endl;
	set<size_t> copied_values;
	for (int i = a; i < b; i++) {
		new_order[i] = this->order[i];
		copied_values.insert(this->order[i]);
	}
	int pos = b%infos.num_cache;
	int pos_other = b%infos.num_cache;
	for (int i = 0; i < infos.num_cache - (b - a); i++) {
		while (copied_values.find(other.order[pos_other]) != copied_values.end()) {
			pos_other++;
			pos_other %= infos.num_cache;
		}
		new_order[pos] = other.order[pos_other];
		pos++;
		pos %= infos.num_cache;
		pos_other++;
		pos_other %= infos.num_cache;
	}
	//for (auto i : order) {
	//	cout << i << "\t";
	//}
	//cout << endl;
	//for (auto i : other.order) {
	//	cout << i << "\t";
	//}
	//cout << endl;
	//for (int i = 0; i < infos.num_cache; i++) {
	//	if (i == a || i == b)cout << "^\t";
	//	else cout << " \t";
	//}
	//cout << endl;
	//for (auto i : new_order) {
	//	cout << i << "\t";
	//}
	//cout << endl;
	//cout << endl;
	return make_shared<OrderGene>(infos, new_order);
}

void OrderGene::mutate(std::default_random_engine & random_generator, int num_swap)
{
	uniform_int_distribution<int> distri(0, infos.num_cache - 1);
	FOR(i, num_swap) {
		int a = distri(random_generator);
		int b = distri(random_generator);
		size_t t = order[a];
		order[a] = order[b];
		order[b] = t;
	}
	calculate_score();
}

OrderGene::~OrderGene()
{
}

static void print_scores(const vector<OrderGene::ptr>& genes) {
	for (auto& g : genes) {
		cout << g->get_id() << "\t" << g->get_score() << endl;
	}
}
vector<OrderGene::ptr> genetic_algo(const Infos& infos, std::default_random_engine & random_generator, int num_genes, int num_growth, int num_generations, double mutate_proba)
{

	vector<OrderGene::ptr> genes(num_genes, nullptr);
	for (auto& g : genes) {
		g = make_shared<OrderGene>(infos, random_generator);
	}
	auto sort_fun = [](const OrderGene::ptr& a, const OrderGene::ptr& b)->bool {
		return a->get_score() > b->get_score();
	};

	sort(begin(genes), end(genes), sort_fun);
	cout << "Initialization:" << endl;
	print_scores(genes);
	uniform_int_distribution<int> distri(0, num_genes - 1);
	uniform_real_distribution<double> proba(0, 1);
	FOR(i, num_generations) {
		//vector<OrderGene::ptr> children(num_growth, nullptr);
		FOR(j, num_growth) {
			int a = distri(random_generator);
			int b;
			do {
				b = distri(random_generator);
			} while (b != a);
			auto child = genes[a]->crossover(*genes[b], random_generator);
			if (proba(random_generator) < mutate_proba) {
				child->mutate(random_generator, 5);
			}
			genes.push_back(child);
		}
		sort(begin(genes), end(genes), sort_fun);
		genes.resize(num_genes);
		cout << "Generation " << i << " :" << endl;
		print_scores(genes);
	}
	return genes;
}
