/*
 * c_fuzzy,
 *
 *
 * Copyright (C) 2014 Davide Tateo
 * Versione 1.0
 *
 * This file is part of c_fuzzy.
 *
 * c_fuzzy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * c_fuzzy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with c_fuzzy.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FUZZYCLASS_H_
#define FUZZYCLASS_H_

#include <map>
#include <set>
#include <vector>

#include "FuzzyConstraints.h"

typedef std::set<std::string> VariableList;
typedef std::map<std::string, std::string> ConstantList;
typedef std::pair<VariableList*, ConstantList*> ElementsList;

class FuzzyClass
{
public:
	FuzzyClass(std::string name, FuzzyClass* superClass,
				VariableList* variables, ConstantList* constants,
				FuzzyConstraintsList* features, bool hidden) :
				name(name), superClass(superClass), variables(variables),
				constants(constants), features(features), hidden(hidden)
	{
	}

	inline bool isHidden()
	{
		return hidden;
	}

	inline bool isTrivial()
	{
		return features->size() == 0;
	}

	inline FuzzyConstraintsList* getfeatureList()
	{
		return features;
	}

	inline std::string getName()
	{
		return name;
	}

	inline FuzzyClass* getSuperClass()
	{
		return superClass;
	}

	inline std::string getSuperClassName()
	{
		if(superClass)
		{
			return superClass->getName();
		}
		else
		{
			return "";
		}
	}

	inline bool containsVar(std::string var)
	{
		return (variables->count(var) == 1)
					|| (superClass && superClass->containsVar(var));
	}

	inline VariableList getVars()
	{
		VariableList vars;
		if(superClass)
			vars = superClass->getVars();

		vars.insert(variables->begin(), variables->end());

		return vars;
	}

	~FuzzyClass()
	{
		delete variables;
		delete constants;
		delete features;
	}

private:
	std::string name;
	FuzzyClass* superClass;
	VariableList* variables;
	ConstantList* constants;
	FuzzyConstraintsList* features;
	bool hidden;
};

#endif /* FUZZYCLASS_H_ */
