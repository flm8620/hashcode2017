#include "knapsack.h"
#include <algorithm>
#include <iostream>
using namespace std;

void knapsack(int N, int W, vector<int>& weight, vector<long long>& cost, std::set<int>& solution)
{
	vector<vector<long long>> c(N + 1);
	for (auto& cc : c) cc.resize(W + 1, 0);

	for (int i = 0; i < N; ++i)
		for (int j = 0; j <= W; ++j)
			if (j - weight[i] < 0)
				// too heavy, can't put
				c[i + 1][j] = c[i][j];
			else/* if (j - weight[i] >= 0)*/
				// can put or not
				c[i + 1][j] = max(
					c[i][j],
					c[i][j - weight[i]] + cost[i]
				);

	//cout << "highest value = " << c[N][W];

	solution.clear();
	// go back to find out decisions
	for (int i = N - 1, j = W; i >= 0; --i)
		// sack may have this item
		if (j - weight[i] >= 0 &&
			c[i + 1][j] == c[i][j - weight[i]] + cost[i])
		{
			solution.insert(i);
			//cout << "sack has item " << i << endl;
			j -= weight[i];
		}
}