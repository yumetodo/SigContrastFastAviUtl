#pragma once
#include "LUT.h"
namespace old {
	class RSigmoidTable :
		public LUT
	{
	public:
		RSigmoidTable(float midtone, float strength, int bin, float multiplier);
		~RSigmoidTable();
	};
}
