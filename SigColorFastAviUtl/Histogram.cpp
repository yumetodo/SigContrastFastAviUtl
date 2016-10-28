#include "Histogram.h"
#include <exception>
#include <stdexcept>

template<typename T>
Histogram<T>::Histogram(T * image, size_t w, size_t h, size_t stride, int channels, int min, int step, int bin)
{
	if (image == nullptr)
	{
		throw (std::invalid_argument("Histogram: image pointer cannot be Null pointer.\n"));
	}
	if (w == 0)
	{
		throw (std::invalid_argument("Histogram: w cannot be ZERO.\n"));
	}
	if (h == 0)
	{
		throw (std::invalid_argument("Histogram: h cannot be ZERO.\n"));

	}
	if (stride < sizeof(T)*channels*w)
	{
		throw (std::invalid_argument("Histogram: stride in bytes is smaller than the minimum possible for w and channels.\n"));
	}
	unsigned char* byte_ptr = reinterpret_cast<unsigned char*>(image);
	kmin = std::max(min, std::numeric_limits<T>().min());
	kstep = step;
	kmax = kmin + kstep*bin;
	// populate the independent variable
	for (int i = kmin; i <= kmax; i++)
	{
		table.insert({ i, 0 });
	}
	// count pixels, row-wise
	double lb, ub, d;
	size_t accumulator{ 0 };
	for (auto& b : table)
	{
		d = static_cast<double>(step) * 0.5;
		lb = static_cast<double>(b.first) - d;
		ub = static_cast<double>(b.first) + d;
		//loop row-wise to handle stride
		for (int r = 0; r < h; r++)
		{
			T* row_ptr = reinterpret_cast<T*>(byte_ptr + stride*r);
			for (int c = 0; c < w; c++)
			{
				double sample = static_cast<double>(*row_ptr);
				if ((sample >= lb) && (sample < ub)) // [a, b)[b, c)
				{
					accumulator++;
				}
				row_ptr += channels;
			}
		} // end of row
		b.second = static_cast<int>(accumulator); //set freq
		accumulator = 0;
	}

}


