#include "RSigmoidTable.h"
#include <xmmintrin.h>
//#define NOMINMAX
//#include <Windows.h>


namespace old {
	RSigmoidTable::RSigmoidTable(float midtone, float strength, int bin, float multiplier)
	{
		kstep = static_cast<int>(multiplier / static_cast<float>(bin));
		kmin = kstep / 2;
		kmax = kmin + kstep*bin;

		for (int x = kmin; x <= kmax; x += kstep)
		{
			float u = static_cast<float>(x) / multiplier;
			float a = midtone;
			float b = strength;
			//float e1, e2, e3, e4;
			float result[4]{ 0 };
			result[0] = std::exp(b*(a - u));
			result[1] = std::exp(a*b);
			result[2] = std::exp(b*(a - 1));
			result[3] = result[1];

			__m128 ei = _mm_loadu_ps(result);
			__m128 v1 = _mm_set1_ps(1.0f);
			__m128 dst = _mm_add_ps(ei, v1);
			ei = _mm_div_ps(v1, dst);

			_mm_storeu_ps(result, ei);
			float y = (result[0] - result[1]) / (result[2] - result[3]);
			y *= multiplier;
			table.insert({ static_cast<int>(y), x });
		}
		kmin = table.begin()->first;
		kmax = table.rbegin()->first;
		kstep = (table.begin()++)->first - kmin;
	}

	RSigmoidTable::~RSigmoidTable()
	{
		table.clear();

		//MessageBox(NULL, "~RSigTable", "Dtor", MB_OK | MB_ICONINFORMATION);
	}
}
