#include "no_min_max.h"
#include "../3rd_party/iutest/include/iutest.hpp"
#include "SigmoidTable.h"
#include "RSigmoidTable.h"
#include "../SigColorFastAviUtl/SigmoidTable.h"
#include "../SigColorFastAviUtl/RSigmoidTable.h"
IUTEST_TEST(SigmoidTableCompatibility, SigmoidTable_test) {
	//0.0 <= midtone <= 1.0, 1.0 <= strength <= 30.0
	SigmoidTable new_table;
	for (int m = 0; m <= 100; ++m) {
		for (int s = 1; s <= 30; ++s) {
			old::SigmoidTable old_table(m / 100.0f, static_cast<float>(s), 4096, 4096.0);
			new_table.change_param(m / 100.0f, static_cast<float>(s));
			for (int i = 0; i <= 4096; ++i) {
				IUTEST_ASSERT_NEAR(static_cast<float>(old_table.lookup(i)), static_cast<float>(new_table.lookup(i)), 1.0f)
					<< " (when i=" << i << " m=" << m << " s=" << s << ')';
			}
		}
	}
}
IUTEST_TEST(SigmoidTableCompatibility, RSigmoidTable_test) {
	//0.0 <= midtone <= 1.0, 1.0 <= strength <= 30.0
	RSigmoidTable new_table;
	for (int m = 0; m <= 100; ++m) {
		for (int s = 1; s <= 30; ++s) {
			old::RSigmoidTable old_table(m / 100.0f, static_cast<float>(s), 4096, 4096.0);
			new_table.change_param(m / 100.0f, static_cast<float>(s));
			for (int i = 0; i <= 4096; ++i) {
				IUTEST_EXPECT_NEAR(static_cast<float>(old_table.lookup(i)), static_cast<float>(new_table.lookup(i)), 2.0f);
				IUTEST_ASSERT_NEAR(static_cast<float>(old_table.lookup(i)), static_cast<float>(new_table.lookup(i)), 3.0f);
			}
		}
	}
}
int main(int argc, char** argv)
{
	IUTEST_INIT(&argc, argv);
	return IUTEST_RUN_ALL_TESTS();
}
