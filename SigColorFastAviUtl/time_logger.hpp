#pragma once
#include <chrono>
#include <deque>
#include <string>
#include <fstream>
#include <cmath>
#include "filter_helper.hpp"
#include "analyzer.hpp"


class time_logger {
public:
	using logging_unit = std::chrono::nanoseconds;
private:
	using rep = logging_unit::rep;
	std::deque<rep> logbuf_;
	const char* filter_name_;
	const char* log_file_name_;
	int w_;
	int h_;
public:
	time_logger() = delete;
	time_logger(const time_logger&) = delete;
	time_logger(time_logger&&) = default;
	time_logger& operator=(const time_logger&) = delete;
	time_logger& operator=(time_logger&&) = default;
	time_logger(const char* filter_name, const char* log_file_name) : logbuf_(), filter_name_(filter_name), log_file_name_(log_file_name), w_(), h_() {}
	void push(const logging_unit& ns, const FILTER_PROC_INFO* fpip) {
		this->logbuf_.push_back(ns.count());
		if (this->w_ != fpip->w) {
			this->w_ = fpip->w;
			this->clear();
		}
		if (this->h_ != fpip->h) {
			this->h_ = fpip->h;
			this->clear();
		}
	}
	template<typename rep, typename period, std::enable_if_t<!std::is_same<std::chrono::duration<rep, period>, logging_unit>::value, std::nullptr_t> = nullptr>
	void push(const std::chrono::duration<rep, period>& time, const FILTER_PROC_INFO* fpip) {
		this->push(std::chrono::duration_cast<logging_unit>(time), fpip);
	}
private:
	analyzer<rep> analyze() { return this->logbuf_; }
public:
	void write_out(const filter_proxy& fc) {
		if (this->logbuf_.size() < 20) return;
		this->logbuf_.pop_front();//remove first data
		std::ofstream logfile(log_file_name_);
		logfile
			<< "image size," << this->w_ << 'x' << this->h_ << std::endl
			<< fc
			<< this->analyze();
		for (auto&& i : this->logbuf_) {
			logfile << filter_name_ << ',' << i << std::endl;
		}
		this->clear();
	}
	void write_out(FILTER* fp) { return write_out(filter_proxy(fp)); }
	void clear(){ this->logbuf_.clear(); }
};
