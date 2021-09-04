#include "no_min_max.h"
#ifndef IUTEST_USE_MAIN
#define IUTEST_USE_MAIN
#endif
#include "../3rd_party/iutest/include/iutest.hpp"
#include "SigmoidTable.h"
#include "RSigmoidTable.h"
#include "analyzer.hpp"
#include "../SigColorFastAviUtl/SigmoidTable.hpp"
#include "../SigColorFastAviUtl/RSigmoidTable.hpp"
#include "../SigColorFastAviUtl/sigmoid.hpp"
#include "../SigColorFastAviUtl/inferior_iota_view.hpp"
#include "../SigColorFastAviUtl/analyzer.hpp"
#include "random.hpp"
#include <iostream>
#include <algorithm>
#include <execution>
#include <ranges>

thread_local auto engine = create_engine();

#if 0
IUTEST_TEST(SigmoidTableCompatibility, SigmoidTable_test) {
	//0.0 <= midtone <= 1.0, 1.0 <= strength <= 30.0
	SigmoidTable new_table;
	for (int m = 0; m <= 100; ++m) {
		for (int s = 1; s <= 30; ++s) {
			old::SigmoidTable old_table(m / 100.0f, static_cast<float>(s), 4096, 4096.0);
			new_table.change_param(m / 100.0f, static_cast<float>(s));
			constexpr auto r = inferior::views::iota(0, 4097);
			std::for_each(std::execution::par, r.begin(), r.end(), [&new_table, &old_table, s, m](int i) {
				IUTEST_ASSERT(0 <= new_table.lookup(i) && new_table.lookup(i) <= 4096);
				IUTEST_EXPECT(old_table.lookup(i) == new_table.lookup(i))
					<< " (when i=" << i << " m=" << m << " s=" << s << ')';
				IUTEST_ASSERT_NEAR(static_cast<float>(old_table.lookup(i)), static_cast<float>(new_table.lookup(i)), 1.0f);
			});
		}
		std::cerr << "\033[0;35mm=" << m + 1 << "/100\033[0;0m\r" << std::flush;
	}
}
IUTEST_TEST(SigmoidTableCompatibility, RSigmoidTable_test) {
	//0.0 <= midtone <= 1.0, 1.0 <= strength <= 30.0
	RSigmoidTable new_table;
	new_table.change_param(1.0f, 30.0f);
	for (int m = 0; m <= 100; ++m) {
		for (int s = 1; s <= 30; ++s) {
			old::RSigmoidTable old_table(m / 100.0f, static_cast<float>(s), 4096, 4096.0);
			new_table.change_param(m / 100.0f, static_cast<float>(s));
			constexpr auto r = inferior::views::iota(0, 4097);
			std::for_each(std::execution::par, r.begin(), r.end(), [&new_table, &old_table, s, m](int i) {
				IUTEST_ASSERT(0 <= new_table.lookup(i) && new_table.lookup(i) <= 4096);
				IUTEST_EXPECT(old_table.lookup(i) == new_table.lookup(i))
					<< " (when i=" << i << " m=" << m << " s=" << s << ')';
				IUTEST_ASSERT_NEAR(static_cast<float>(old_table.lookup(i)), static_cast<float>(new_table.lookup(i)), 1.0f);
			});
		}
		std::cerr << "\033[0;35mm=" << m + 1 << "/100\033[0;0m\r" << std::flush;
	}
}
#endif

IUTEST_TEST(SigmoidTableCompatibility, SigmoidTable_test_out_of_range) {
	//0.0 <= midtone <= 1.0, 1.0 <= strength <= 30.0
	SigmoidTable new_table;
	for (int m = 0; m <= 100; ++m) {
		for (int s = 1; s <= 30; ++s) {
			old::SigmoidTable old_table(m / 100.0f, static_cast<float>(s), 4096, 4096.0);
			new_table.change_param(m / 100.0f, static_cast<float>(s));
			for (int i : {-1, 4097}) {
				IUTEST_EXPECT(old_table.lookup(i) == new_table.lookup(i))
					<< " (when i=" << i << " m=" << m << " s=" << s << ')';
				//IUTEST_ASSERT_NEAR(static_cast<float>(old_table.lookup(i)), static_cast<float>(new_table.lookup(i)), 1.0f);
			}
		}
	}
}
IUTEST_TEST(SigmoidTableCompatibility, RSigmoidTable_test_out_of_range) {
	//0.0 <= midtone <= 1.0, 1.0 <= strength <= 30.0
	RSigmoidTable new_table;
	new_table.change_param(1.0f, 30.0f);
	for (int m = 0; m <= 100; ++m) {
		for (int s = 1; s <= 30; ++s) {
			old::RSigmoidTable old_table(m / 100.0f, static_cast<float>(s), 4096, 4096.0);
			new_table.change_param(m / 100.0f, static_cast<float>(s));
			for (int i : {-1, 4097}) {
				IUTEST_EXPECT(old_table.lookup(i) == new_table.lookup(i))
					<< " (when i=" << i << " m=" << m << " s=" << s << ')';
				//IUTEST_ASSERT_NEAR(static_cast<float>(old_table.lookup(i)), static_cast<float>(new_table.lookup(i)), 1.0f);
			}
		}
	}
}
IUTEST_TEST(SigmoidCompatibility, sigmod_test) {
	static std::uniform_real_distribution<float> midtone(0.0, 1.0);
	static std::uniform_real_distribution<float> strength(1.0, 30.0);
	static std::uniform_int_distribution<std::uint16_t> u(0, 4096);
	auto make_input = []() { return static_cast<float>(u(engine)) / 4096.0f; };
	for (unsigned int i = 0; i < 100; ++i) {
		const auto m = midtone(engine);
		const auto s = strength(engine);
		const auto u = make_input();
		IUTEST_EXPECT(sigmoid(m, s, u) == sigmoid(m, s, u, sigmoid_pre(m, s)));
	}
	for (float m : {0.0f, 1.0f}) for (float s : {1.0f, 30.0f}) for (std::uint16_t u : {0, 4096}) {
		IUTEST_EXPECT(sigmoid(m, s, static_cast<float>(u) / 4096.0f) == sigmoid(m, s, static_cast<float>(u) / 4096.0f, sigmoid_pre(m, s)));
	}
}

IUTEST_TEST(Analyzer, calcAverage) {
	using rep = std::chrono::nanoseconds::rep;
	std::deque<rep> input;
	input.resize(1000);
	std::uniform_int_distribution<rep> d(1000, 7880500);
	for (auto _ : std::views::iota(0, 1000)) input.emplace_back(d(engine));
	old_accumulate::analyzer old(input);
	analyzer current(input);
	IUTEST_EXPECT_DOUBLE_EQ(old.average, current.average);
}

IUTEST_TEST(Analyzer, calcStdev) {
	using rep = std::chrono::nanoseconds::rep;
	std::deque<rep> input;
	std::uniform_int_distribution<rep> d(1000, 7880500);
	for (auto _ : std::views::iota(0, 1000)) input.emplace_back(d(engine));
	old_accumulate::analyzer old(input);
	analyzer current(input);
	IUTEST_EXPECT_DOUBLE_EQ(old.stdev, current.stdev);
}
