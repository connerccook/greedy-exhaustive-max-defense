

#include "maxdefense.hh"
#include "timer.hh"

int main()
{
	auto all_armors = load_armor_database("armor.csv");
	assert( all_armors );
	std::unique_ptr<ArmorVector> soln_greedy, soln_exhaustive;
	auto filtered_armors = filter_armor_vector(*all_armors, 1, 2500, 6);
	Timer timer;
	//soln_greedy = greedy_max_defense(*filtered_armors, 1000);
	soln_exhaustive = exhaustive_max_defense(*filtered_armors,500);
	double elapsed = timer.elapsed();
	std::cout << elapsed << std::endl;
	return 0;
}


