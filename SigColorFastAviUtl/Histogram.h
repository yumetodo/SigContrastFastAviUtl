#pragma once
#include "LUT.h"
#include <limits>
template<typename T> class Histogram :
	public LUT
{
public:
	Histogram(T* image, size_t w, size_t h, size_t stride, int channels=1, int min= std::numeric_limits<T>().min(), int step=1, 
		int bin= std::min(std::numeric_limits<T>().max()- std::numeric_limits<T>().min(), 65535));
	~Histogram() = default;

};

