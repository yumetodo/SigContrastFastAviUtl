#pragma once
#include <cstdint>
#include <array>
#include <algorithm>
#include "sigmoid.hpp"
#include "thread.hpp"
class RSigmoidTable
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
	void insert_to_table(std::size_t pos, value_type value) noexcept;
public:
	RSigmoidTable() = default;
	RSigmoidTable(const RSigmoidTable&) = delete;
	RSigmoidTable(RSigmoidTable&&) = delete;
	RSigmoidTable& operator=(const RSigmoidTable&) = delete;
	RSigmoidTable& operator=(RSigmoidTable&&) = delete;
	void change_param(float midtone, float strength) noexcept;

	template<typename Unsigned, std::enable_if_t<std::is_unsigned<Unsigned>::value && (sizeof(value_type) <= sizeof(Unsigned)), std::nullptr_t> = nullptr>
	value_type lookup(Unsigned key) const noexcept { return (table_size <= key) ? table_.back() : table_[key]; }

	template<typename T, std::enable_if_t<
		(std::is_signed<T>::value && (sizeof(value_type) <= sizeof(T))) || std::is_floating_point<T>::value, 
	std::nullptr_t> = nullptr>
	value_type lookup(T key) const noexcept { return (key < 0 || static_cast<T>(table_size) <= key) ? table_.back() : table_[static_cast<value_type>(key)]; }
};
inline void RSigmoidTable::insert_to_table(std::size_t pos, value_type value) noexcept
{
	//to avoid multiple insertation, check target element whether this is first insert.
	if (0 != pos && 0 == table_[pos]) table_[pos] = value;
}
inline void RSigmoidTable::change_param(float midtone, float strength) noexcept
{
	//0.0 <= midtone <= 1.0, 1.0 <= strength <= 30.0
	if (midtone == this->midtone_ && strength == this->strength_) return;
	//clear array for checking multiple insertation at function `RSigmoidTable::insert_to_table`
	std::fill(table_.begin(), table_.end(), 0);
	for (value_type pre = 0, y = 1; y <= bin; ++y)
	{
		const auto x = static_cast<value_type>(multiplier * sigmoid(midtone, strength, static_cast<float>(y) / multiplier));
		for (value_type i = pre + 1; i <= x; ++i) this->insert_to_table(i, y);//fill blanc and insert new value
		pre = x;
	}
}

