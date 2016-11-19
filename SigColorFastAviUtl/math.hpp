#pragma once
#include <limits>
#include <type_traits>
#include <stdexcept>
namespace math {
	namespace detail {
		template<typename T1, typename T2>
		struct max_type : std::conditional<(sizeof(T1) < sizeof(T2)), T2, T1> {};
		template<typename T1, typename T2>
		using max_type_t = typename max_type<T1, T2>::type;
		template<typename T1, typename T2, std::enable_if_t<sizeof(T1) == sizeof(T2), std::nullptr_t> = nullptr>
		constexpr bool abs_diff_can_noexcept() {
			return std::numeric_limits<max_type_t<T1, T2>>::min() + 1 == -std::numeric_limits<max_type_t<T1, T2>>::max()
				|| std::numeric_limits<max_type_t<T1, T2>>::min() == -std::numeric_limits<max_type_t<T1, T2>>::max();
		}
		template<typename T1, typename T2, std::enable_if_t<sizeof(T1) != sizeof(T2), std::nullptr_t> = nullptr>
		constexpr bool abs_diff_can_noexcept() {
			return std::numeric_limits<max_type_t<T1, T2>>::min() + 1 == -std::numeric_limits<max_type_t<T1, T2>>::max()
				|| std::numeric_limits<max_type_t<T1, T2>>::min() == -std::numeric_limits<max_type_t<T1, T2>>::max();
		}
		/**
		* @param a bigger unsigned num
		* @param b smaller signed negative num
		*/
		template <typename T1, typename T2, std::enable_if_t<
			std::is_unsigned<T1>::value && std::is_signed<T2>::value,
			std::nullptr_t
		> = nullptr>
		static inline auto abs_diff_impl(const T1& a, const T2& b) noexcept(detail::abs_diff_can_noexcept<T1, T2>())
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
			//note: a is positive
			return (-lim::max() <= b)
				//``-b`` is no problem
				? static_cast<utype>(a) + static_cast<std::make_unsigned_t<T2>>(-b)
				//std::numeric_limits<T>::min() <= b < -std::numeric_limits<T>::max()
				: 
				(
					(lim::max() < (ulim::max() - static_cast<utype>(a)))
					//	(try to store rest) (----------------storable max num----------------)
					&& ((b - lim::min()) <= (ulim::max() - lim::max() - static_cast<utype>(a)))
				)
					//can store
					? static_cast<utype>(a) + static_cast<utype>(lim::max()) + static_cast<utype>((-lim::max()) - b)
					//when processing system doesn't use two's complement and 
					//std::numeric_limits<T>::min() < -std::numeric_limits<T>::max()
					//there is possibility no way to store result.
					//In that case, we throw exception.
					: throw std::invalid_argument();
		}
	}
	template <typename T1, typename T2, std::enable_if_t<
		std::is_signed<T1>::value && std::is_signed<T2>::value,
		std::nullptr_t
	> = nullptr>
	static inline constexpr auto abs_diff(const T1& a, const T2& b) noexcept(detail::abs_diff_can_noexcept<T1, T2>::value)
		->std::make_unsigned_t<std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, T1>>
	{
		using lim = std::numeric_limits<T2>;
		using utype = std::make_unsigned_t<std::conditional_t<(sizeof(T1) < sizeof(T2)), T2, T1>>;
		using ulim = std::numeric_limits<utype>;
		return (b < a)
			? abs_diff(a, b)
			: (b <= 0 || lim::min() + b <= a)
				? static_cast<utype>(b - a)
				: detail::abs_diff_impl(static_cast<utype>(b), a);
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
			: (0 <= b)
				? a - static_cast<std::make_unsigned_t<T2>>(b)
				: detail::abs_diff_impl(a, b);
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
