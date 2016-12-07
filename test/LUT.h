#pragma once
#include <map>
#include <stdexcept>
#include <exception>
#include <memory>
#include <algorithm>
#include <cmath>

/* Base class for Histogram and SigGraph */
/* Use integer calculation for performance */
class LUT
{
public:
	LUT();
	virtual ~LUT();
	int lookup(int key);
	//Getter
	int min() const;
	int max() const;
	int step() const;
	//Setter
	void min(int key);
	void max(int key);
	void step(int delta);
protected:
	std::map<int, int> table;
	int kmin, kmax, kstep;
};

