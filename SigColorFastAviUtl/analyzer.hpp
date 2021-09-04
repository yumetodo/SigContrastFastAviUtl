#pragma once
#include <chrono>
#include <deque>
#include <string>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <execution>
#include <future>
#include "filter_helper.hpp"
template<typename T>
struct analyzer {
	using value_type = T;
	std::size_t count;
	double average;
	double stdev;
	double se;
	double confidence_95;
	double confidence_95_min;
	double confidence_95_max;
	value_type min;
	value_type max;
private:
	static value_type calc_sum(const std::deque<value_type>& logbuf) {
		return std::reduce(std::execution::par, logbuf.begin(), logbuf.end());
	}
	static double calc_average(value_type sum, std::size_t count) {
		const auto [quot, rem] = std::div(sum, value_type(count));
		return double(quot) + double(rem) / count;
	}
	static double calc_stdev_impl(double n, double) { return n; }
	static double calc_stdev_impl(value_type n, double average)
	{
		const auto re = static_cast<double>(n) - average;
		return re * re;
	}
	static double calc_stdev(const std::deque<value_type>& logbuf, double average) {
		return std::sqrt(
			std::reduce(std::execution::par, logbuf.begin(), logbuf.end(), 0.0, [average](auto l, auto r) {
				return calc_stdev_impl(l, average) + calc_stdev_impl(r, average);
			}) / (logbuf.size() - 1)
		);
	}
	static double calc_se(double stdev, std::size_t count) {
		return stdev / std::sqrt(count);
	}
	static double calc_confidence_95(double se) {
		return 1.959964 * se;
	}
	static std::pair<double, double> calc_confidence_interval(double average, double confidence) {
		return{ average - confidence, average + confidence };
	}
	static std::pair<value_type, value_type>  minmax(const std::deque<value_type>& logbuf) {
		const auto it_minmax = std::minmax_element(logbuf.begin(), logbuf.end());
		return{ *it_minmax.first, *it_minmax.second };
	}
	analyzer() = delete;
public:
	analyzer(const analyzer&) = default;
	analyzer(analyzer&&) = default;
	analyzer& operator=(const analyzer&) = default;
	analyzer& operator=(analyzer&&) = default;
	analyzer(const std::deque<value_type>& logbuf) : count(logbuf.size()) {
		auto minmax_th = std::async([&logbuf]() { return minmax(logbuf); });
		average = calc_average(calc_sum(logbuf), count);
		stdev = calc_stdev(logbuf, average);
		se = calc_se(stdev, count);
		confidence_95 = calc_confidence_95(se);
		std::tie(confidence_95_min, confidence_95_max) = calc_confidence_interval(average, confidence_95);
		std::tie(min, max) = minmax_th.get();
	}
};
template<typename T>
inline std::ostream& operator<<(std::ostream& os, const analyzer<T>& analyzed) {
	using std::endl;
	using std::fixed;
	using std::setprecision;
	os
		<< "N," << analyzed.count << endl
		<< "max," << analyzed.max << endl
		<< "min," << analyzed.min << endl;
	os
		<< "avg,"		<< fixed << setprecision(7) << analyzed.average << endl
		<< "stdev,"		<< fixed << setprecision(7) << analyzed.stdev << endl
		<< "se,"		<< fixed << setprecision(7) << analyzed.se << endl
		<< "95%CI,"		<< fixed << setprecision(7) << analyzed.confidence_95 << endl
		<< "95%CI.max,"	<< fixed << setprecision(7) << analyzed.confidence_95_max << endl
		<< "95%CI.min,"	<< fixed << setprecision(7) << analyzed.confidence_95_min << endl;
	return os;
}
