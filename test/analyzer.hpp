﻿#pragma once
#include <chrono>
#include <deque>
#include <string>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <future>

namespace old_accumulate {
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
		static double calc_sum(const std::deque<value_type>& logbuf) {
			return std::accumulate(logbuf.begin(), logbuf.end(), 0.0);
		}
		static double calc_average(double sum, std::size_t count) {
			return sum / count;
		}
		static double calc_stdev(const std::deque<value_type>& logbuf, double average) {
			return std::sqrt(
				std::accumulate(logbuf.begin(), logbuf.end(), 0.0, [average](double sum, value_type val) {
					return sum + std::pow(static_cast<double>(val) - average, 2);
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
}