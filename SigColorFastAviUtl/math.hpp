#pragma once
#include <limits>
#include <type_traits>
#include <stdexcept>
namespace math {
	namespace detail {
		constexpr bool is_two_s_complement() noexcept {
			return std::numeric_limits<int>::min() + 1 == -std::numeric_limits<int>::max();
		}
		constexpr bool is_one_s_complement_like() noexcept {
			return std::numeric_limits<int>::min() == -std::numeric_limits<int>::max();
		}
		constexpr bool abs_diff_both_signed_can_noexcept() noexcept {
			return is_two_s_complement() || is_one_s_complement_like();
		}
		/**
		* @param a bigger unsigned num
		* @param b smaller signed negative num
		*/
		template <typename T1, typename T2, std::enable_if_t<
			std::is_unsigned<T1>::value && std::is_signed<T2>::value,
			std::nullptr_t
		> = nullptr>
		static inline constexpr auto abs_diff_impl(const T1& a, const T2& b)
			->std::make_unsigned_t<std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, T1>>
		{
			using lim = std::numeric_limits<T2>;
			using utype = std::make_unsigned_t<std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, T1>>;
			using ulim = std::numeric_limits<utype>;
			//prevent overflow
			//http://qiita.com/a4lg/items/bc4d2cfbce22fe749589
			//-std::numeric_limits<T>::min() < std::numeric_limits<T>::max() : iregal after C99
			//std::numeric_limits<T>::min() < -std::numeric_limits<T>::max() : most familiar behavior
			//std::numeric_limits<T>::min() = -std::numeric_limits<T>::max() : possible
			//note: 0 <= a, b < 0
			//
			//lim::min()   -lim::max()          0                  a            lim::max()
			//    |             |               |                  |                |
			//----+-------------+-----.......---+------.......-----+----.......-----+-----
			return (-lim::max() <= b)
				//``-b`` is no problem
				//
				//lim::min()   -lim::max()          b                  0
				//    |             |               |                  |
				//----+-------------+-----.......---+------.......-----+----.......
				? static_cast<utype>(a) + static_cast<std::make_unsigned_t<T2>>(-b)
				//std::numeric_limits<T>::min() <= b < -std::numeric_limits<T>::max()
				//lim::min()        b          -lim::max()             0
				//    |             |               |                  |
				//----+-------------+-----.......---+------.......-----+----.......
				:
				(
					(static_cast<utype>(lim::max()) < (ulim::max() - static_cast<utype>(a)))
					//  (---------try to store rest---------)    (----------------storable max num----------------)
					&& (static_cast<utype>((-lim::max()) - b) <= (ulim::max() - lim::max() - static_cast<utype>(a)))
				)
					//can store
					? static_cast<utype>(a) + static_cast<utype>(lim::max()) + static_cast<utype>((-lim::max()) - b)
					//when processing system doesn't use two's complement and 
					//std::numeric_limits<T>::min() < -std::numeric_limits<T>::max()
					//there is possibility no way to store result.
					//In that case, we throw exception.
					: throw std::invalid_argument("cannot store result.");
		}
	}
	template <typename T1, typename T2, std::enable_if_t<
		std::is_signed<T1>::value && std::is_signed<T2>::value,
		std::nullptr_t
	> = nullptr>
	static inline constexpr auto abs_diff(const T1& a, const T2& b) noexcept(detail::abs_diff_both_signed_can_noexcept())
		->std::make_unsigned_t<std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, T1>>
	{
		using lim = std::numeric_limits<T2>;
		using utype = std::make_unsigned_t<std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, T1>>;
		return (b < a)
			? abs_diff(b, a)
			: (b <= 0 || lim::min() + b <= a)
				? static_cast<utype>(b - a)
				: detail::abs_diff_impl(static_cast<utype>(b), a);
	}
	template <typename T1, typename T2, std::enable_if_t<
		std::is_unsigned<T1>::value && std::is_signed<T2>::value,
		std::nullptr_t
	> = nullptr>
	static inline constexpr auto abs_diff(const T1& a, const T2& b)
		->std::conditional_t<(sizeof(T1) < sizeof(T2)), std::make_unsigned_t<T2>, T1>
	{
		return (0 < b && a < static_cast<std::make_unsigned_t<T2>>(b))
			? static_cast<std::make_unsigned_t<T2>>(b) - a
			: (0 <= b)
				? a - static_cast<std::make_unsigned_t<T2>>(b)
				: detail::abs_diff_impl(a, b);
	}
	template <typename T1, typename T2, std::enable_if_t<
		std::is_signed<T1>::value && std::is_unsigned<T2>::value,
		std::nullptr_t
	> = nullptr>
	static inline constexpr auto abs_diff(const T1& a, const T2& b)
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
