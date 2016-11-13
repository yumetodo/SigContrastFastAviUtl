#pragma once
#include <chrono>
#include <deque>
#include <string>
#include <fstream>
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
	void write_out() {
		if (this->logbuf_.empty()) return;
		std::ofstream logfile(log_file_name_);
		for (auto&& i : this->logbuf_) {
			logfile << filter_name_ << ',' << i << std::endl;
		}
	}
};