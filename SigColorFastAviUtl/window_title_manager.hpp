#pragma once
#include "filter_helper.hpp"
#include <chrono>
#include <string>
namespace detail {
	namespace ch = std::chrono;
	using namespace std::chrono_literals;
	class window_title_manager {
	public:
		using print_unit = ch::microseconds;
		using timepoint = ch::time_point<ch::steady_clock>;
	private:
		timepoint last_echo_time_;
		std::string plugin_name_;
		window_title_manager() = delete;
	public:
		window_title_manager(const char* plugin_name) : last_echo_time_(ch::steady_clock::now()), plugin_name_(plugin_name){};
		window_title_manager(const window_title_manager&) = default;
		window_title_manager(window_title_manager&&) = default;
		window_title_manager& operator=(const window_title_manager&) = default;
		window_title_manager& operator=(window_title_manager&&) = default;
		void notify_print_time(const filter_proxy& fc, const FILTER_PROC_INFO* fpip, const timepoint& time, const print_unit& elapsed) {
			if (last_echo_time_ + 160ms < time) {
				const auto elapsed_s = std::to_string(elapsed.count());
				::SetWindowText(fc.window_handle(), (plugin_name_ + ":" + elapsed_s + "micro sec. @" + std::to_string(fpip->w) + "x" + std::to_string(fpip->h)).c_str());
				fc.notify_update_window();
				last_echo_time_ = time;
			}
		}
		template<typename rep, typename period, std::enable_if_t<!std::is_same<std::chrono::duration<rep, period>, print_unit>::value, std::nullptr_t> = nullptr>
		void notify_print_time(const filter_proxy& fc, const FILTER_PROC_INFO* fpip, const timepoint& time, const ch::duration<rep, period>& elapsed) {
			notify_print_time(fc, fpip, time, ch::duration_cast<print_unit>(elapsed));
		}
		void print_default(const filter_proxy& fc) {
			::SetWindowText(fc.window_handle(), plugin_name_.c_str());
			fc.notify_update_window();
		}
	};
}
using detail::window_title_manager;
