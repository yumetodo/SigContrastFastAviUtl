#pragma once
#include "LUT.h"
namespace old {
	class SigmoidTable :
		public LUT
	{
	public:
		SigmoidTable(float midtone, float strength, int bin, float multiplier);
		~SigmoidTable();
	};
}
