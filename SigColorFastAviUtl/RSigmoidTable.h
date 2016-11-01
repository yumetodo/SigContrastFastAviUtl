#pragma once
#include "LUT.h"
class RSigmoidTable :
	public LUT
{
public:
	RSigmoidTable(float midtone, float strength, int bin, float multiplier);
	~RSigmoidTable();
};

