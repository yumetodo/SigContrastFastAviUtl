/*=============================================================================
Copyright (C) 2021 yumetodo <yume-wikijp@live.jp>
Distributed under the Boost Software License, Version 1.0.
(See http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef YUMETODO_INFERIOR_IOTA_VIEW_HPP_
#define YUMETODO_INFERIOR_IOTA_VIEW_HPP_
#include <cstddef>
#include <iterator>
namespace inferior {
	namespace ranges {
		namespace detail {
			template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, std::nullptr_t> = nullptr>
			class iota_view_iterator {
			private:
				T i;
			public:
				using iterator_category = std::random_access_iterator_tag;
				using iterator_concept = std::random_access_iterator_tag;
				using value_type = T;
				using difference_type = std::ptrdiff_t;
				using pointer = T*;
				using reference = T;

				iota_view_iterator() = default;
				constexpr iota_view_iterator(T n) noexcept : i(n) { }
				iota_view_iterator(const iota_view_iterator&) = default;
				iota_view_iterator(iota_view_iterator&&) = default;
				iota_view_iterator& operator=(const iota_view_iterator&) = default;
				iota_view_iterator& operator=(iota_view_iterator&&) = default;
				constexpr bool operator == (const iota_view_iterator& rhs) const noexcept { return i == rhs.i; }
				constexpr bool operator != (const iota_view_iterator& rhs) const noexcept { return i != rhs.i; }
				constexpr T operator * () const noexcept { return i; }
				constexpr T operator[](difference_type n) const noexcept { return static_cast<T>(i + n); }
				constexpr iota_view_iterator operator+(difference_type n) const noexcept { return{ static_cast<T>(i + n) }; }
				constexpr iota_view_iterator operator-(difference_type n) const noexcept { return{ static_cast<T>(i - n) }; }
				constexpr difference_type operator-(const iota_view_iterator& it) const noexcept { return static_cast<difference_type>(i - it.i); }
				constexpr bool operator<(const iota_view_iterator& n) const noexcept { return i < n.i; }
				constexpr bool operator<=(const iota_view_iterator& n) const noexcept { return i <= n.i; }
				constexpr bool operator>(const iota_view_iterator& n) const noexcept { return i > n.i; }
				constexpr bool operator>=(const iota_view_iterator& n) const noexcept { return i >= n.i; }
				constexpr iota_view_iterator& operator += (iota_view_iterator it) noexcept {
					i = static_cast<T>(i + it.i);
					return *this;
				}
				constexpr iota_view_iterator& operator -= (iota_view_iterator it) noexcept {
					i = static_cast<T>(i - it.i);
					return *this;
				}
				constexpr iota_view_iterator& operator ++ () noexcept {
					++i;
					return *this;
				}
				constexpr iota_view_iterator operator ++ (int) noexcept {
					auto t = *this;
					++i;
					return t;
				}
				constexpr iota_view_iterator& operator -- () noexcept {
					--i;
					return *this;
				}
				constexpr iota_view_iterator operator -- (int) noexcept {
					auto t = *this;
					--i;
					return t;
				}
			};
			template<typename T>
			constexpr iota_view_iterator<T> operator+(typename iota_view_iterator<T>::difference_type n, const iota_view_iterator<T>& it) noexcept { return static_cast<T>(it + n); }
		}
		template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, std::nullptr_t> = nullptr>
		class iota_view {
		public:
			using value_type = std::remove_cv_t<T>;
			using iterator = detail::iota_view_iterator<value_type>;
		private:
			value_type begin_, end_;
		public:
			constexpr iota_view(T n) : begin_(0), end_(n) {}
			constexpr iota_view(T begin, T end) : begin_(begin), end_(end) {}
			constexpr iterator begin() const noexcept {
				return{ begin_ };
			}
			constexpr iterator end() const noexcept {
				return{ end_ };
			}
		};

		namespace views {
			template<typename T, std::enable_if_t<std::is_arithmetic<T>::value, std::nullptr_t> = nullptr>
			constexpr iota_view<T> iota(const T& begin, const T& end) noexcept {
				return{ begin, end };
			}
		}
	}
	namespace views = ranges::views;
}
#endif //YUMETODO_INFERIOR_IOTA_VIEW_HPP_
