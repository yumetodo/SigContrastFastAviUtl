#include "no_min_max.h"
#include "../3rd_party/iutest/include/iutest.hpp"
#include "SigmoidTable.h"
#include "RSigmoidTable.h"
#include "../SigColorFastAviUtl/SigmoidTable.hpp"
#include "../SigColorFastAviUtl/RSigmoidTable.hpp"
#include "../SigColorFastAviUtl/sigmoid.hpp"
#include "random.hpp"

static auto engine = create_engine();

IUTEST_TEST(SigmoidTableCompatibility, SigmoidTable_test) {
	//0.0 <= midtone <= 1.0, 1.0 <= strength <= 30.0
	SigmoidTable new_table;
	for (int m = 0; m <= 100; ++m) {
		for (int s = 1; s <= 30; ++s) {
			old::SigmoidTable old_table(m / 100.0f, static_cast<float>(s), 4096, 4096.0);
			new_table.change_param(m / 100.0f, static_cast<float>(s));
			for (int i = 0; i <= 4096; ++i) {
				IUTEST_ASSERT(0 <= new_table.lookup(i) && new_table.lookup(i) <= 4096);
				IUTEST_EXPECT(old_table.lookup(i) == new_table.lookup(i))
					<< " (when i=" << i << " m=" << m << " s=" << s << ')';
				IUTEST_ASSERT_NEAR(static_cast<float>(old_table.lookup(i)), static_cast<float>(new_table.lookup(i)), 1.0f);
			}
		}
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
			for (int i = 0; i <= 4096; ++i) {
				IUTEST_ASSERT(0 <= new_table.lookup(i) && new_table.lookup(i) <= 4096);
				IUTEST_EXPECT(old_table.lookup(i) == new_table.lookup(i))
					<< " (when i=" << i << " m=" << m << " s=" << s << ')';
				IUTEST_ASSERT_NEAR(static_cast<float>(old_table.lookup(i)), static_cast<float>(new_table.lookup(i)), 1.0f);
			}
		}
	}
}
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

template<typename T>
struct uniform_int_distribution : std::uniform_int_distribution<T> {
	using base_type = std::uniform_int_distribution<T>;
	using result_type = typename base_type::result_type;
	explicit uniform_int_distribution(result_type a = 0, result_type b = std::numeric_limits<result_type>::max())
		: base_type(a, b)
	{}
};
template<>
struct uniform_int_distribution<std::int8_t> : std::uniform_int_distribution<std::int16_t> {
	using result_type = std::int8_t;
	using base_type = std::uniform_int_distribution<std::int16_t>;
	explicit uniform_int_distribution(result_type a = 0, result_type b = std::numeric_limits<result_type>::max())
		: base_type(a, b)
	{}
	template <class URNG>
	result_type operator()(URNG& g)
	{
		return static_cast<result_type>(static_cast<base_type&>(*this)(g));
	}

	template <class URNG>
	result_type operator()(URNG& g, const param_type& parm)
	{
		return static_cast<result_type>(static_cast<base_type&>(*this)(g, parm));
	}
};
template<>
struct uniform_int_distribution<std::uint8_t> : std::uniform_int_distribution<std::uint16_t> {
	using result_type = std::uint8_t;
	using base_type = std::uniform_int_distribution<std::uint16_t>;
	explicit uniform_int_distribution(result_type a = 0, result_type b = std::numeric_limits<result_type>::max())
		: base_type(a, b)
	{}
	template <class URNG>
	result_type operator()(URNG& g)
	{
		return static_cast<result_type>(static_cast<base_type&>(*this)(g));
	}

	template <class URNG>
	result_type operator()(URNG& g, const param_type& parm)
	{
		return static_cast<result_type>(static_cast<base_type&>(*this)(g, parm));
	}
};

template<typename T>
struct abs_diff_test : public ::iutest::Test {};
IUTEST_TYPED_TEST_CASE(abs_diff_test, ::iutest::Types<std::int8_t, std::int16_t, std::int32_t, std::int64_t>);
IUTEST_TYPED_TEST(abs_diff_test, both_positive)
{
	using type = TypeParam;
	using utype = std::make_unsigned_t<type>;
	static uniform_int_distribution<type> rand(0, std::numeric_limits<type>::max());
	for (unsigned int i = 0; i < 100; ++i) {
		utype re[8] = {};
		const type in[2] = { rand(engine), rand(engine) };
		const utype uin[2] = { static_cast<utype>(in[0]), static_cast<utype>(in[1]) };
		re[0] = math::abs_diff(in[0], in[1]);
		re[1] = math::abs_diff(in[0], uin[1]);
		re[2] = math::abs_diff(uin[0], in[1]);
		re[3] = math::abs_diff(uin[0], uin[1]);
		re[4] = math::abs_diff(in[1], in[0]);
		re[5] = math::abs_diff(in[1], uin[0]);
		re[6] = math::abs_diff(uin[1], in[0]);
		re[7] = math::abs_diff(uin[1], uin[0]);
		IUTEST_EXPECT(re[0] == re[1]);
		IUTEST_EXPECT(re[1] == re[2]);
		IUTEST_EXPECT(re[2] == re[3]);
		IUTEST_EXPECT(re[3] == re[4]);
		IUTEST_EXPECT(re[4] == re[5]);
		IUTEST_EXPECT(re[5] == re[6]);
		IUTEST_EXPECT(re[6] == re[7]);
	}
	utype re[8] = {};
	constexpr type in[2] = { type(0), std::numeric_limits<type>::max() };
	constexpr utype uin[2] = { utype(0), static_cast<utype>(in[1]) };
	re[0] = math::abs_diff(in[0], in[1]);
	re[1] = math::abs_diff(in[0], uin[1]);
	re[2] = math::abs_diff(uin[0], in[1]);
	re[3] = math::abs_diff(uin[0], uin[1]);
	re[4] = math::abs_diff(in[1], in[0]);
	re[5] = math::abs_diff(in[1], uin[0]);
	re[6] = math::abs_diff(uin[1], in[0]);
	re[7] = math::abs_diff(uin[1], uin[0]);
	IUTEST_EXPECT(re[0] == re[1]);
	IUTEST_EXPECT(re[1] == re[2]);
	IUTEST_EXPECT(re[2] == re[3]);
	IUTEST_EXPECT(re[0] == re[1]);
	IUTEST_EXPECT(re[1] == re[2]);
	IUTEST_EXPECT(re[2] == re[3]);
	IUTEST_EXPECT(re[3] == re[4]);
	IUTEST_EXPECT(re[4] == re[5]);
	IUTEST_EXPECT(re[5] == re[6]);
	IUTEST_EXPECT(re[6] == re[7]);
	IUTEST_EXPECT(re[7] == uin[1]);
}
IUTEST_TYPED_TEST(abs_diff_test, positive_and_negative)
{
	using type = TypeParam;
	using utype = std::make_unsigned_t<type>;
	static uniform_int_distribution<type> positive(0, std::numeric_limits<type>::max());
	static uniform_int_distribution<type> negative(-std::numeric_limits<type>::max(), -1);
	for (unsigned int i = 0; i < 100; ++i) {
		utype re[6] = {};
		const type in[2] = { positive(engine), negative(engine) };
		const utype uin = static_cast<utype>(in[0]);
		re[0] = math::abs_diff(in[0], in[1]);
		re[1] = math::abs_diff(in[1], uin);
		re[2] = math::abs_diff(uin, in[1]);
		re[3] = math::abs_diff(in[1], in[0]);
		re[4] = math::abs_diff(in[1], uin);
		re[5] = math::abs_diff(uin, in[1]);
		IUTEST_EXPECT(re[0] == re[1]);
		IUTEST_EXPECT(re[1] == re[2]);
		IUTEST_EXPECT(re[2] == re[3]);
		IUTEST_EXPECT(re[3] == re[4]);
		IUTEST_EXPECT(re[4] == re[5]);
	}
}
IUTEST_TYPED_TEST(abs_diff_test, positive_and_negative_error)
{
	using type = TypeParam;
	using utype = std::make_unsigned_t<type>;
	static_assert(std::is_unsigned<utype>::value, "err");
	static uniform_int_distribution<type> negative(-std::numeric_limits<type>::max(), -1);
	for (unsigned int i = 0; i < 1000; ++i) {
		const type in_negative = negative(engine);
		uniform_int_distribution<utype> positive(
			std::numeric_limits<utype>::max() - static_cast<utype>(-in_negative) + 1,
			std::numeric_limits<utype>::max()
		);
		const utype in_positive = positive(engine);
		IUTEST_EXPECT_THROW(math::abs_diff(in_negative, in_positive), std::invalid_argument)
			<< "in_negative:" << static_cast<std::intmax_t>(in_negative) 
			<< " in_positive:" << static_cast<std::uintmax_t>(in_positive);
		IUTEST_EXPECT_THROW(math::abs_diff(in_positive, in_negative), std::invalid_argument);
	}
}
IUTEST_TYPED_TEST(abs_diff_test, both_negative)
{
	using type = TypeParam;
	using utype = std::make_unsigned_t<type>;
	static uniform_int_distribution<type> rand(1, std::numeric_limits<type>::max());
	for (unsigned int i = 0; i < 100; ++i) {
		utype re[3] = {};
		const type in[2] = { rand(engine), rand(engine) };
		re[0] = math::abs_diff(static_cast<utype>(in[0]), static_cast<utype>(in[1]));
		re[1] = math::abs_diff(-in[0], -in[1]);
		re[2] = math::abs_diff(-in[1], -in[0]);
		IUTEST_EXPECT(re[0] == re[1]);
		IUTEST_EXPECT(re[1] == re[2]);
	}
}
IUTEST_TYPED_TEST(abs_diff_test, other)
{
	using type = TypeParam;
	using utype = std::make_unsigned_t<type>;
	constexpr type zero = 0;
	constexpr utype uzero = 0;
	IUTEST_EXPECT(uzero == math::abs_diff(zero, zero));
	IUTEST_EXPECT(uzero == math::abs_diff(zero, uzero));
	IUTEST_EXPECT(uzero == math::abs_diff(uzero, zero));
	IUTEST_EXPECT(uzero == math::abs_diff(uzero, uzero));
	constexpr type inputs[] = { 
		std::numeric_limits<type>::min(), std::numeric_limits<type>::max(), -std::numeric_limits<type>::max() 
	};
	for (type i : inputs) {
		IUTEST_EXPECT(uzero == math::abs_diff(i, i));
	}
	if (math::detail::is_two_s_complement()) {
		IUTEST_EXPECT(std::numeric_limits<utype>::max() == math::abs_diff(std::numeric_limits<type>::min(), std::numeric_limits<type>::max()));
	}
	else if (math::detail::is_one_s_complement_like()) {
		IUTEST_EXPECT((std::numeric_limits<utype>::max() - 1) == math::abs_diff(std::numeric_limits<type>::min(), std::numeric_limits<type>::max()));
	}
	else {
		IUTEST_EXPECT_THROW(math::abs_diff(std::numeric_limits<type>::min(), std::numeric_limits<type>::max()), std::invalid_argument);
	}
}
int main(int argc, char** argv)
{
	IUTEST_INIT(&argc, argv);
	return IUTEST_RUN_ALL_TESTS();
}
