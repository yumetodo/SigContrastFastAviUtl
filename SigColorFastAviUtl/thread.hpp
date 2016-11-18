#pragma once
#include <thread>
#include <type_traits>
#include <vector>
#include <limits>
#include <future>
#include <iterator>
namespace parallel {
	namespace detail {
		template <typename T1, typename T2, std::enable_if_t<
			std::is_signed<T1>::value && std::is_signed<T2>::value,
			std::nullptr_t
		> = nullptr>
		static inline constexpr auto abs_diff(const T1& a, const T2& b) noexcept
			-> std::make_unsigned_t<std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, T1>>
		{
			return (b < a) 
				? abs_diff(a, b)
				: (b <= 0 || std::numeric_limits<T2>::min() + b <= a) 
					? static_cast<std::make_unsigned_t<std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, T1>>>(b - a)
					//prevent overflow
					: (std::numeric_limits<T2>::min() != a)
						? static_cast<std::make_unsigned_t<T2>>(b) + static_cast<std::make_unsigned_t<T2>>(-a)
						: (-std::numeric_limits<T2>::max() == std::numeric_limits<T2>::min() + 1)
							? std::numeric_limits<std::make_unsigned_t<T2>>::min()
							: throw std::invalid_argument();
		}
		template <typename T1, typename T2, std::enable_if_t<
			std::is_unsigned<T1>::value && std::is_signed<T2>::value,
			std::nullptr_t
		> = nullptr>
		static inline constexpr auto abs_diff(const T1& a, const T2& b) noexcept
			->std::conditional_t<(sizeof(T1) < sizeof(T2)), std::make_unsigned_t<T2>, T1>
		{
			return (a < b)
				? static_cast<std::make_unsigned_t<T2>>(b) - a
				: (b < 0)
					? a + static_cast<std::make_unsigned_t<T2>>(-b)
					: a - static_cast<std::make_unsigned_t<T2>>(b);
		}
		template <typename T1, typename T2, std::enable_if_t<
			std::is_signed<T1>::value && std::is_unsigned<T2>::value,
			std::nullptr_t
		> = nullptr>
		static inline constexpr auto abs_diff(const T1& a, const T2& b) noexcept
			->std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, std::make_unsigned_t<T1>>
		{
			return abs_diff(b, a);
		}
		template <typename T1, typename T2, std::enable_if_t<
			std::is_unsigned<T1>::value && std::is_unsigned<T2>::value,
			std::nullptr_t
		> = nullptr>
		static inline constexpr auto abs_diff(const T1& a, const T2& b) noexcept
			->std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, T1>
		{
			return (a < b) ? b - a : a - b;
		}
	}
	template<
		typename Index, typename Func,
		typename ...Args,
		std::enable_if_t<std::is_integral<Index>::value, std::nullptr_t> = nullptr
	>
	inline void par_for(Index begin, Index end, Func&& f, Args&& ...args) {
		const unsigned int thread_num = std::thread::hardware_concurrency();
		if (thread_num < 2) {//thread 非対応
			f(begin, end, std::forward<Args>(args)...);
		}
		else {
			const auto num = parallel::detail::abs_diff(end, begin);
			const auto task_num = num / thread_num;
			const auto task_rest = num % thread_num;
			std::vector<std::thread> th;
			th.reserve(thread_num);
			for (unsigned int i = 0; i < thread_num; ++i) {
				th.emplace_back(
					std::forward<Func>(f),
					(i) ? static_cast<Index>(i * task_num + task_rest + begin) : begin,
					static_cast<Index>((i + 1) * task_num + task_rest + begin),
					std::forward<Args>(args)...
				);
			}
			for (auto&& t : th) t.join();
		}
	}
	template<
		typename Index, typename Func,
		typename RawIndexType = Index,
		typename ...Args,
		std::enable_if_t<std::is_unsigned<Index>::value && std::is_integral<RawIndexType>::value, std::nullptr_t> = nullptr
	>
	inline void par_for(Index num, Func&& f, Args&& ...args) {
		const unsigned int thread_num = std::thread::hardware_concurrency();
		if (thread_num < 2) {//thread 非対応
			f(static_cast<RawIndexType>(0), static_cast<RawIndexType>(num), std::forward<Args>(args)...);
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
					static_cast<RawIndexType>((i + 1) * task_num + task_rest),
					std::forward<Args>(args)...
				);
			}
			for (auto&& t : th) t.join();
		}
	}
	template<typename Index, typename Func, typename ...Args, std::enable_if_t<std::is_signed<Index>::value, std::nullptr_t> = nullptr>
	inline void par_for(Index num, Func&& f, Args&& ...args) {
		if (0 < num) par_for<std::uintmax_t, Func, Index, Args...>(static_cast<std::uintmax_t>(num), std::forward<Func>(f), std::forward<Args>(args)...);
	}
	template<
		typename RandomAccessIterator, typename Func, typename ...Args,
		std::enable_if_t<
			std::is_same<std::random_access_iterator_tag, typename std::iterator_traits<RandomAccessIterator>::iterator_category>::value, 
			std::nullptr_t
		> = nullptr
	>
	inline auto async_for(RandomAccessIterator begin, RandomAccessIterator end, Func&& f, Args&& ...args)
		-> std::vector<std::future<std::result_of_t<std::decay_t<Func>(RandomAccessIterator, RandomAccessIterator, std::decay_t<Args>...)>>>
	{
		const unsigned int thread_num = std::thread::hardware_concurrency();
		std::vector<std::future<std::result_of_t<std::decay_t<Func>(RandomAccessIterator, RandomAccessIterator, std::decay_t<Args>...)>>> re;
		if (thread_num < 2) {//thread 非対応
			re.push_back(std::async(std::launch::deferred, f, begin, end, std::forward<Args>(args)...));
		}
		else {
			const auto num = std::distance(begin, end);
			const auto task_num = num / thread_num;
			const auto task_rest = num % thread_num;
			re.reserve(thread_num);
			for (unsigned int i = 0; i < thread_num; ++i) {
				re.push_back(std::async(
					std::launch::async,
					std::forward<Func>(f),
					(i) ? begin + (i * task_num + task_rest) : begin,
					begin + ((i + 1) * task_num + task_rest),
					std::forward<Args>(args)...
				));
			}
		}
		return re;
	}
	template<
		typename Index, typename Func,
		typename ...Args,
		std::enable_if_t<std::is_integral<Index>::value, std::nullptr_t> = nullptr
	>
	inline auto async_for(Index begin, Index end, Func&& f, Args&& ...args) 
		-> std::vector<std::future<std::result_of_t<std::decay_t<Func>(Index, Index, std::decay_t<Args>...)>>>
	{
		const unsigned int thread_num = std::thread::hardware_concurrency();
		std::vector<std::future<std::result_of_t<std::decay_t<Func>(Index, Index, std::decay_t<Args>...)>>> re;
		if (thread_num < 2) {//thread 非対応
			re.push_back(std::async(std::launch::deferred, f, begin, end, std::forward<Args>(args)...));
		}
		else {
			const auto num = parallel::detail::abs_diff(end, begin);
			const auto task_num = num / thread_num;
			const auto task_rest = num % thread_num;
			re.reserve(thread_num);
			for (unsigned int i = 0; i < thread_num; ++i) {
				re.push_back(std::async(
					std::launch::async,
					std::forward<Func>(f),
					(i) ? static_cast<Index>(i * task_num + task_rest + begin) : begin,
					static_cast<Index>((i + 1) * task_num + task_rest + begin),
					std::forward<Args>(args)...
				));
			}
		}
		return re;
	}
	template<
		typename Index, typename Func,
		typename ...Args,
		std::enable_if_t<std::is_integral<Index>::value, std::nullptr_t> = nullptr
	>
	inline auto async_for(Index num, Func&& f, Args&& ...args)
		-> std::vector<std::future<std::result_of_t<std::decay_t<Func>(Index, Index, std::decay_t<Args>...)>>>
	{
		return async_for(static_cast<Index>(0), num, std::forward<Func>(f), std::forward<Args>(args)...);
	}

}
