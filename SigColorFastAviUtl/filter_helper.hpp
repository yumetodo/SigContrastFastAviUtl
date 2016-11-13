#pragma once
#include <cstddef>
#include <cstdint>
#include "filter.h"
enum class track : std::uint8_t {
	midtone = 0,
	strength = 1
};
enum class check : std::uint8_t {
	Y = 0,
	R = 1,
	G = 2,
	B = 3,
#ifdef USECLOCK
	echo_benchmark = 4,
	save_benchmark = 5,
	disable_benchmark_during_export = 6,
#endif
};
enum filter_update_track_status : int {
	FILTER_UPDATE_MIDTONE_TRACK = FILTER_UPDATE_STATUS_TRACK + static_cast<int>(track::midtone),
	FILTER_UPDATE_STRENGTH_TRACK = FILTER_UPDATE_STATUS_TRACK + static_cast<int>(track::strength),
};
enum filter_update_check_status : int {
	FILTER_UPDATE_Y_CHECK = FILTER_UPDATE_STATUS_CHECK + static_cast<int>(check::Y),
	FILTER_UPDATE_R_CHECK = FILTER_UPDATE_STATUS_CHECK + static_cast<int>(check::R),
	FILTER_UPDATE_G_CHECK = FILTER_UPDATE_STATUS_CHECK + static_cast<int>(check::G),
	FILTER_UPDATE_B_CHECK = FILTER_UPDATE_STATUS_CHECK + static_cast<int>(check::B),
#ifdef USECLOCK
	FILTER_UPDATE_ECHO_BENCHMARK_CHECK = FILTER_UPDATE_STATUS_CHECK + static_cast<int>(check::echo_benchmark),
#endif
};
class filter_proxy {
	filter_proxy() = delete;
	filter_proxy(filter_proxy&&) = delete;
	filter_proxy& operator=(const filter_proxy&) = delete;
	filter_proxy& operator=(filter_proxy&&) = delete;
	FILTER* fp_;
public:
	filter_proxy(const filter_proxy&) = default;
	explicit filter_proxy(FILTER* fp) : fp_(fp) {}
	bool any_of(check c) const noexcept { return 0 != this->fp_->check[static_cast<std::size_t>(c)]; }
	template<typename T, typename... Rest, std::enable_if_t<std::is_same<T, check>::value, std::nullptr_t> = nullptr>
	bool any_of(T flag, Rest&&... rest) const noexcept { return any_of(flag) || any_of(std::forward<Rest>(rest)...); }
	template<typename... Args>
	bool none_of(Args&&... args) const noexcept { return !any_of(std::forward<Args>(args)...); }
	int operator[](track t) const noexcept { return this->fp_->track[static_cast<std::size_t>(t)]; }
	bool operator[](check c) const noexcept { return any_of(c); }
	int& operator[](track t) noexcept { return this->fp_->track[static_cast<std::size_t>(t)]; }
	int& operator[](check c) noexcept { return this->fp_->check[static_cast<std::size_t>(c)]; }
	void set_rgb(bool state) {
		(*this)[check::R] = (*this)[check::G] = (*this)[check::B] = state;
	}
	void disable_benchmark() noexcept {
		(*this)[check::echo_benchmark] = (*this)[check::save_benchmark] = false;
	}
	BOOL notify_update_window() const noexcept{ return this->fp_->exfunc->filter_window_update(this->fp_); }
	HWND window_handle() const { return this->fp_->hwnd; }
};
