///////////////////////////////////////////////////////////////////////////////
// maxdefense.hh
//
// Compute the set of armos that maximizes defense, within a gold budget,
// with the greedy method or exhaustive search.
//
///////////////////////////////////////////////////////////////////////////////


#pragma once


#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <vector>
#include "timer.hh"

class ArmorItem
{
	//
	public:

		//
		ArmorItem
		(
			const std::string& description,
			double cost_gold,
			double defense_points
		)
			:
			_description(description),
			_cost_gold(cost_gold),
			_defense_points(defense_points)
		{
			assert(!description.empty());
			assert(cost_gold > 0);
		}

		//
		const std::string& description() const { return _description; }
		double cost() const { return _cost_gold; }
		double defense() const { return _defense_points; }

	//
	private:

		// Human-readable description of the armor, e.g. "new enchanted helmet". Must be non-empty.
		std::string _description;

		// Cost, in units of gold; Must be positive
		double _cost_gold;

		// Defense points; most be non-negative.
		double _defense_points;
};


// Alias for a vector of shared pointers to ArmorItem objects.
typedef std::vector<std::shared_ptr<ArmorItem>> ArmorVector;


// Load all the valid armor items from the CSV database
// Armor items that are missing fields, or have invalid values, are skipped.
// Returns nullptr on I/O error.
std::unique_ptr<ArmorVector> load_armor_database(const std::string& path)
{
	std::unique_ptr<ArmorVector> failure(nullptr);

	std::ifstream f(path);
	if (!f)
	{
		std::cout << "Failed to load armor database; Cannot open file: " << path << std::endl;
		return failure;
	}

	std::unique_ptr<ArmorVector> result(new ArmorVector);

	size_t line_number = 0;
	for (std::string line; std::getline(f, line); )
	{
		line_number++;

		// First line is a header row
		if ( line_number == 1 )
		{
			continue;
		}

		std::vector<std::string> fields;
		std::stringstream ss(line);

		for (std::string field; std::getline(ss, field, '^'); )
		{
			fields.push_back(field);
		}

		if (fields.size() != 3)
		{
			std::cout
				<< "Failed to load armor database: Invalid field count at line " << line_number << "; Want 3 but got " << fields.size() << std::endl
				<< "Line: " << line << std::endl
				;
			return failure;
		}

		std::string
			descr_field = fields[0],
			cost_gold_field = fields[1],
			defense_points_field = fields[2]
			;

		auto parse_dbl = [](const std::string& field, double& output)
		{
			std::stringstream ss(field);
			if ( ! ss )
			{
				return false;
			}

			ss >> output;

			return true;
		};

		std::string description(descr_field);
		double cost_gold, defense_points;
		if (
			parse_dbl(cost_gold_field, cost_gold)
			&& parse_dbl(defense_points_field, defense_points)
		)
		{
			result->push_back(
				std::shared_ptr<ArmorItem>(
					new ArmorItem(
						description,
						cost_gold,
						defense_points
					)
				)
			);
		}
	}

	f.close();

	return result;
}


// Convenience function to compute the total cost and defense in an ArmorVector.
// Provide the ArmorVector as the first argument
// The next two arguments will return the cost and defense back to the caller.
void sum_armor_vector
(
	const ArmorVector& armors,
	double& total_cost,
	double& total_defense
)
{
	total_cost = total_defense = 0;
	for (auto& armor : armors)
	{
		total_cost += armor->cost();
		total_defense += armor->defense();
	}
}


// Convenience function to print out each ArmorItem in an ArmorVector,
// followed by the total kilocalories and protein in it.
void print_armor_vector(const ArmorVector& armors)
{
	std::cout << "*** Armor Vector ***" << std::endl;

	if ( armors.size() == 0 )
	{
		std::cout << "[empty armor list]" << std::endl;
	}
	else
	{
		for (auto& armor : armors)
		{
			std::cout
				<< "Ye olde " << armor->description()
				<< " ==> "
				<< "Cost of " << armor->cost() << " gold"
				<< "; Defense points = " << armor->defense()
				<< std::endl
				;
		}

		double total_cost, total_defense;
		sum_armor_vector(armors, total_cost, total_defense);
		std::cout
			<< "> Grand total cost: " << total_cost << " gold" << std::endl
			<< "> Grand total defense: " << total_defense
			<< std::endl
			;
	}
}


// Filter the vector source, i.e. create and return a new ArmorVector
// containing the subset of the armor items in source that match given
// criteria.
// This is intended to:
//	1) filter out armor with zero or negative defense that are irrelevant to our optimization
//	2) limit the size of inputs to the exhaustive search algorithm since it will probably be slow.
//
// Each armor item that is included must have at minimum min_defense and at most max_defense.
//	(i.e., each included armor item's defense must be between min_defense and max_defense (inclusive).
//
// In addition, the the vector includes only the first total_size armor items that match these criteria.
std::unique_ptr<ArmorVector> filter_armor_vector
(
	const ArmorVector& source,
	double min_defense,
	double max_defense,
	int total_size
)
{
	std::unique_ptr<ArmorVector> filtered = std::make_unique<ArmorVector>();

	for(auto armor : source){
		if ( armor->defense() >= min_defense && armor->defense() <= max_defense){
			filtered->push_back(armor);
			if (filtered->size() >= total_size){
				break;
			}
		}
	}

	return filtered;
}


// Compute the optimal set of armor items with a greedy algorithm.
// Specifically, among the armor items that fit within a total_cost gold budget,
// choose the armors whose defense is greatest.
// Repeat until no more armor items can be chosen, either because we've run out of armor items,
// or run out of gold.
std::unique_ptr<ArmorVector> greedy_max_defense
(
	const ArmorVector& armors,
	double total_cost
)
{
	std::unique_ptr<ArmorVector> result = std::make_unique<ArmorVector>();
	std::unique_ptr<ArmorVector> todo(new ArmorVector(armors));

	double result_cost = 0.0; //Variable to keep track of the total armor cost

	while (todo->size() != 0){
		size_t max_armor_index = -1;
	    std::shared_ptr<ArmorItem> max_armor = todo->at(0);
		double max_armor_value = 0.0;

		for(size_t i = 0; i < todo->size(); i++ ){
			auto armor = todo->at(i);

			if((result_cost + armor->cost()) <= total_cost){ //Condition to check if total armor cost goes over budget
			double armor_value = armor->defense() / armor->cost();
			if ( armor_value > max_armor_value ) //Compares the armor values
				{
					max_armor = armor;
					max_armor_value = armor_value;
					max_armor_index =i;
				}
			}
		}
		if (max_armor_index == -1) { break;}

		result->push_back(max_armor); //Pushes best armor item into a vector
		result_cost += max_armor->cost(); //Adds armor cost to total cost
		todo->erase(todo->begin() + max_armor_index); //Removes the armor
	}
	return result;
}


// Compute the optimal set of armor items with an exhaustive search algorithm.
// Specifically, among all subsets of armor items,
// return the subset whose gold cost fits within the total_cost budget,
// and whose total defense is greatest.
// To avoid overflow, the size of the armor items vector must be less than 64.
std::unique_ptr<ArmorVector> exhaustive_max_defense
(
	const ArmorVector& armors,
	double total_cost
)
{
	const int n = armors.size();
	assert(n < 64);
	double cost=0.0;
	double defense=0.0;
	double best_defense = 0.0;
	
	std::unique_ptr<ArmorVector> bestVector(new ArmorVector);
	std::unique_ptr<ArmorVector> candidates(new ArmorVector);

	bool flag = false;

	for (uint64_t bits = 0; bits < pow(2,n); bits++){
		candidates->clear();
		for(u_int64_t j = 0; j < n; j++){
			if(((bits >> j) & 1) == 1){
				candidates->push_back(armors[j]);
			}
		}
		sum_armor_vector(*candidates,cost,defense);
		if(cost <= total_cost){
			if (!flag || defense > best_defense) {
			best_defense = defense;
			*bestVector = *candidates;
			flag = true;
			}
		}
	}
	return bestVector;
}
