#pragma once
#include <chrono>
#include <deque>
#include <string>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include "filter_helper.hpp"
#include "thread.hpp"
#include "std_future.hpp"
namespace detail {
	template<typename ResultType, typename Container, typename F>
	inline ResultType accumulate_mt(const Container& c, ResultType init_val, F&& f) {
		auto handle = parallel::async_for(std::begin(c), std::end(c), [init_val](auto begin, auto end, F&& f) {
			return std::accumulate(begin, end, init_val, std::forward<F>(f));
		}, std::forward<F>(f));
		return std::accumulate(handle.begin(), handle.end(), init_val, [](ResultType sum, std::future<ResultType>& val) {
			return sum + val.get();
		});
	}
	template<typename ResultType, typename Container>
	inline ResultType accumulate_mt(const Container& c, ResultType init_val) {
		auto handle = parallel::async_for(std::begin(c), std::end(c), [init_val](auto begin, auto end) {
			return std::accumulate(begin, end, init_val);
		});
		return std::accumulate(handle.begin(), handle.end(), init_val, [](ResultType sum, std::future<ResultType>& val) {
			return sum + val.get();
		});
	}
	template<typename ResultType, typename Container, typename F>
	inline ResultType accumulate(const Container& c, ResultType init_val, F&& f) {
		return (500 < std_future::size(c)) ? accumulate_mt(c, init_val, std::forward<F>(f)) : std::accumulate(c.begin(), c.end(), init_val, std::forward<F>(f));
	}
	template<typename ResultType, typename Container>
	inline ResultType accumulate(const Container& c, ResultType init_val){
		return (500 < std_future::size(c)) ? accumulate_mt(c, init_val) : std::accumulate(c.begin(), c.end(), init_val);
	}
}
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
		return detail::accumulate(logbuf, 0.0);
	}
	static double calc_average(double sum, std::size_t count) {
		return sum / count;
	}
	static double calc_stdev(const std::deque<value_type>& logbuf, double average) {
		return std::sqrt(
			detail::accumulate(logbuf, 0.0, [average](double sum, value_type val) {
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

class time_logger {
public:
	using logging_unit = std::chrono::nanoseconds;
private:
	using rep = logging_unit::rep;
	std::deque<rep> logbuf_;
	const char* filter_name_;
	const char* log_file_name_;
public:
	time_logger() = delete;
	time_logger(const time_logger&) = delete;
	time_logger(time_logger&&) = default;
	time_logger& operator=(const time_logger&) = delete;
	time_logger& operator=(time_logger&&) = default;
	time_logger(const char* filter_name, const char* log_file_name) : logbuf_(), filter_name_(filter_name), log_file_name_(log_file_name) {}
	void push(const logging_unit& ns) {
		this->logbuf_.push_back(ns.count());
	}
	template<typename rep, typename period, std::enable_if_t<!std::is_same<std::chrono::duration<rep, period>, logging_unit>::value, std::nullptr_t> = nullptr>
	void push(const std::chrono::duration<rep, period>& time) {
		this->push(std::chrono::duration_cast<logging_unit>(time));
	}
private:
	analyzer<rep> analyze() { return this->logbuf_; }
public:
	void write_out(const filter_proxy& fc) {
		if (this->logbuf_.empty()) return;
		std::ofstream logfile(log_file_name_);
		logfile 
			<< fc
			<< this->analyze();
		for (auto&& i : this->logbuf_) {
			logfile << filter_name_ << ',' << i << std::endl;
		}
		this->logbuf_.clear();
	}
	void write_out(FILTER* fp) { return write_out(filter_proxy(fp)); }
	void clear(){ this->logbuf_.clear(); }
};
