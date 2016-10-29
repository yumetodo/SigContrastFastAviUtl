#pragma once
#include "LUT.h"
class SigmoidTable :
	public LUT
{
public:
	SigmoidTable(float midtone, float strength, int bin, float multiplier);
	~SigmoidTable();
};

