#include "ordergene.h"

#include "connectionstatus.h"
using namespace std;

int OrderGene::id_count = 0;
void OrderGene::calculate_score()
{
	ConnectionStatus cs(infos);
	cs.method1_fill_cache(order, 0);
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

OrderGene::OrderGene(const OrderGene & other) : infos(other.infos), id(id_count++)
{
	order = other.order;
	score = other.score;
}

OrderGene::ptr OrderGene::clone()
{
	auto c = make_shared<OrderGene>(*this);
	return c;
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
	return make_shared<OrderGene>(infos, new_order);
}

int OrderGene::mutate(std::default_random_engine & random_generator, double continue_proba)
{
	uniform_int_distribution<int> distri(0, infos.num_cache - 1);
	uniform_real_distribution<double> proba(0, 1);
	int count = 0;
	do {
		int a = distri(random_generator);
		int b = distri(random_generator);
		size_t t = order[a];
		order[a] = order[b];
		order[b] = t;
		count++;
	} while (proba(random_generator) < continue_proba);
	calculate_score();
	return count;
}

int OrderGene::mutate2(std::default_random_engine & random_generator, double continue_proba)
{
	uniform_int_distribution<int> distri(0, infos.num_cache - 1);
	uniform_real_distribution<double> proba(0, 1);
	int count = 0;
	do {
		int a = distri(random_generator);
		auto temp = order;
		FOR(i, infos.num_cache) {
			order[i] = temp[(a + i) % infos.num_cache];
		}
		count++;
	} while (proba(random_generator) < continue_proba);
	calculate_score();
	return count;
}

OrderGene::~OrderGene()
{
}

static void print_scores(const vector<OrderGene::ptr>& genes) {
	for (auto& g : genes) {
		cout << g->get_id() << "\t" << g->get_score() << endl;
	}
}
void write_file(const vector<OrderGene::ptr>& genes, string filename) {
	ofstream f(filename);
	f << genes.size() << " " << genes[0]->get_order_ref().size() << endl;
	for (auto& g : genes) {
		const auto& order = g->get_order_ref();
		for (size_t i : order)
			f << i << " ";
		f << endl;
	}

	f.close();
}
void read_file_order(std::vector<std::vector<size_t>>& orders, std::string filename)
{
	orders.clear();
	ifstream f(filename);
	if (!f.good()) throw "file error";
	int num_gene, num_cache;
	f >> num_gene >> num_cache;
	FOR(i, num_gene) {
		std::vector<size_t> order(num_cache);
		for (auto& o : order)
			f >> o;
		orders.push_back(order);
	}
	f.close();
}
void read_file(vector<OrderGene::ptr>& genes, string filename, const Infos& infos) {
	genes.clear();
	ifstream f(filename);
	if (!f.good()) throw "file error";
	int num_gene, num_cache;
	f >> num_gene >> num_cache;
	FOR(i, num_gene) {
		std::vector<size_t> order(num_cache);
		for (auto& o : order)
			f >> o;
		auto g = make_shared<OrderGene>(infos, order);
		genes.push_back(g);
	}
	f.close();
}
vector<OrderGene::ptr> genetic_algo(const Infos& infos, std::default_random_engine & random_generator, int num_genes, int num_growth, int num_generations, double mutate_proba, string initial_file)
{
	time_type t0, t1, t2, t3;
	vector<OrderGene::ptr> genes(num_genes, nullptr);
	auto sort_fun = [](const OrderGene::ptr& a, const OrderGene::ptr& b)->bool {
		return a->get_score() > b->get_score();
	};


	if (initial_file.size() > 0) {
		cout << "Reading from file " << initial_file << endl;
		read_file(genes, initial_file, infos);
		if (genes.size() != num_genes) throw "file_wrong";
	}
	else {
		cout << "Initialing..." << endl;
		for (auto& g : genes) {
			cout << "." << flush;
			g = make_shared<OrderGene>(infos, random_generator);
		}
		cout << endl;
		sort(begin(genes), end(genes), sort_fun);
	}
	cout << "Initialization score:" << endl;
	print_scores(genes);
	uniform_int_distribution<int> distri(0, num_genes - 1);
	uniform_real_distribution<double> proba(0, 1);
	FOR(i, num_generations) {
		cout << "Generation " << i << endl;
		//vector<OrderGene::ptr> children(num_growth, nullptr);
		cout << "crossing over" << endl;
		t0 = now();
		FOR(j, num_growth) {
			int a = distri(random_generator);
			int b;
			do {
				b = distri(random_generator);
			} while (b == a);
			auto child = genes[a]->crossover(*genes[b], random_generator);
			cout << genes[a]->get_id() << "\t" << genes[a]->get_score() << "\t+\t" << genes[b]->get_id() << "\t" << genes[b]->get_score() << "\t-->\t";
			cout << child->get_id() << "\t" << child->get_score() << endl;
			genes.push_back(child);
			if (proba(random_generator) < mutate_proba) {
				auto child2 = child->clone();
				int mutate_count = child2->mutate(random_generator, 0.9);
				genes.push_back(child2);
				cout << "\tmutate1 " << mutate_count << ": " << child2->get_id() << "\t\t\t\t" << child2->get_score() << endl;

				auto child3 = child2->clone();
				int mutate2_count = child3->mutate2(random_generator, 0.65);
				genes.push_back(child3);
				cout << "\tmutate2 " << mutate2_count << ": " << child3->get_id() << "\t\t\t\t" << child3->get_score() << endl;
			}

		}
		t1 = now();
		cout << "time = " << timedif(t1, t0) / 1000000.0 << " secs" << endl;
		cout << endl;
		sort(begin(genes), end(genes), sort_fun);
		genes.resize(num_genes);
		cout << "Generation " << i << " :" << endl;
		print_scores(genes);

		string fullname = string("order_file_") + timestamp() + string("_gen_") + to_string(i) + string(".order");
		cout << "Save to file " << fullname << endl;
		write_file(genes, fullname);
	}
	return genes;
}
