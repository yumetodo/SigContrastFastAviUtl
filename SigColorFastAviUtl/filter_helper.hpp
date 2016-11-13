#pragma once
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
