#pragma once
#include <cstdint>
#include <array>
#include <execution>
#include <algorithm>
#include "inferior_iota_view.hpp"
#include "sigmoid.hpp"
class SigmoidTable
{
public:
	using value_type = std::uint16_t;
	static constexpr value_type table_size = 4097;
	static constexpr value_type bin = table_size - 1;
	static constexpr float multiplier = bin;
private:
	std::array<value_type, table_size> table_;
	float midtone_;
	float strength_;
	static constexpr value_type on_error_value_high_ = bin;
	static constexpr value_type on_error_value_low_ = 0;

public:
	SigmoidTable() = default;
	SigmoidTable(const SigmoidTable&) = delete;
	SigmoidTable(SigmoidTable&&) = delete;
	SigmoidTable& operator=(const SigmoidTable&) = delete;
	SigmoidTable& operator=(SigmoidTable&&) = delete;
	void change_param(float midtone, float strength) noexcept;

	template<typename Unsigned, std::enable_if_t<std::is_unsigned<Unsigned>::value && (sizeof(value_type) <= sizeof(Unsigned)), std::nullptr_t> = nullptr>
	value_type lookup(Unsigned key) const noexcept { return (table_size <= key) ? on_error_value_high_ : table_[key]; }

	template<typename T, std::enable_if_t<
		(std::is_signed<T>::value && (sizeof(value_type) <= sizeof(T))) || std::is_floating_point<T>::value,
	std::nullptr_t> = nullptr>
	value_type lookup(T key) const noexcept {
		return (key < 0) ? on_error_value_low_ : (table_size <= static_cast<std::uintmax_t>(key)) ? on_error_value_high_ : table_[static_cast<value_type>(key)];
	}
};
inline void SigmoidTable::change_param(float midtone, float strength) noexcept
{
	//0.0 <= midtone <= 1.0, 1.0 <= strength <= 30.0
	if (midtone == this->midtone_ && strength == this->strength_) return;
	table_.front() = 0;
	constexpr auto r = inferior::views::iota(std::uint16_t{ 1 }, bin);
	const auto pre_sigmoid = sigmoid_pre(midtone, strength);
	std::for_each(std::execution::par, r.begin(), r.end(), [pre_sigmoid, midtone, strength, &t = table_](value_type x) {
		t[x] = static_cast<value_type>(multiplier * sigmoid(midtone, strength, static_cast<float>(x) / multiplier, pre_sigmoid));
	});
	table_.back() = bin;
}

