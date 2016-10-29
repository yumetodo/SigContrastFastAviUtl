#include "LUT.h"
//#define NOMINMAX
//#include <Windows.h>


LUT::LUT(): table{}, kmin{0}, kmax{0}, kstep{1}
{ // Nothing todo
}

LUT::~LUT()
{
	//MessageBox(NULL, "~LUT", "Dtor", MB_OK | MB_ICONINFORMATION);
}




int LUT::lookup(int key)
{
	///* Clamp to min max */
	//if (key > kmax)
	//{
	//	key = kmax;
	//}
	//else if (key < kmin)
	//{
	//	key = kmin;
	//}
	//else
	//{
	//	//OK
	//}
	///** quantize key to the nearest valid value **/
	//key = ((key - kmin) / kstep)*kstep + kmin;
	///* lookup */
	//int ret;
	//try
	//{
	//	ret = table.at(key);
	//}
	//catch (std::out_of_range e)
	//{
	//	ret = kmin;
	//}
	//return ret;
	auto result = table.lower_bound(key);
	if (result != table.end())
	{
		return result->second;
	}
	else
	{
		return table.rbegin()->second;
	}
}

int LUT::min() const
{

	return kmin;
}

int LUT::max() const
{
	return kmax;
}

int LUT::step() const
{
	return kstep;
}

void LUT::min(int key)
{
	kmin = key;
}

void LUT::max(int key)
{
	kmax = key;
}

void LUT::step(int delta)
{
	kstep = delta;
}
