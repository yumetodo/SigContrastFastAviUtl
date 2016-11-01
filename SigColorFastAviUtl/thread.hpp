#pragma once
#include <thread>
#include <type_traits>
#include <vector>
namespace parallel {
	template<
		typename Index, typename Func,
		typename RawIndexType = Index,
		std::enable_if_t<std::is_unsigned<Index>::value && std::is_integral<RawIndexType>::value, std::nullptr_t> = nullptr
	>
	inline void par_for(Index num, Func&& f) {
		const unsigned int thread_num = std::thread::hardware_concurrency();
		if (thread_num < 2) {//thread 非対応
			f(static_cast<RawIndexType>(0), static_cast<RawIndexType>(num));
		}
		else {
			const auto task_num = num / thread_num;
			const auto task_rest = num % thread_num;
			std::vector<std::thread> th;
			th.reserve(thread_num);
			for (unsigned int i = 0; i < thread_num; ++i) {
				th.emplace_back(
					std::forward<Func>(f),
					static_cast<RawIndexType>((i) ? i * task_num + task_rest : 0),
					static_cast<RawIndexType>((i + 1) * task_num + task_rest)
				);
			}
			for (auto&& t : th) t.join();
		}
	}
	template<typename Index, typename Func, std::enable_if_t<std::is_signed<Index>::value, std::nullptr_t> = nullptr>
	inline void par_for(Index num, Func&& f) {
		if (0 < num) par_for<std::uintmax_t, Func, Index, nullptr>(static_cast<std::uintmax_t>(num), std::forward<Func>(f));
	}
}
