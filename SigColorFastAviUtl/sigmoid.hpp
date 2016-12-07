#pragma once
#include <cmath>
#include <xmmintrin.h>
namespace detail {
	inline float sigmoid_impl(__m128 ei) noexcept {
		//float e1, e2, e3, e4;
		float result[4];
		static const __m128 v1 = _mm_set1_ps(1.0f);
		const __m128 dst = _mm_add_ps(ei, v1);
		ei = _mm_div_ps(v1, dst);

		_mm_storeu_ps(result, ei);
		return (result[0] - result[1]) / (result[2] - result[3]);
	}

}
inline float sigmoid(float a, float b, float u) noexcept {
	//float e1, e2, e3, e4;
	float result[4];
	result[0] = std::exp(b*(a - u));
	result[1] = std::exp(a*b);
	result[2] = std::exp(b*(a - 1));
	result[3] = result[1];

	return detail::sigmoid_impl(_mm_loadu_ps(result));
}
inline float sigmoid(float a, float b, float u, __m128 pre) noexcept {
	//float e1, e2, e3, e4;
	const __m128 tmp = _mm_set_ss(std::exp(b*(a - u)));
	return detail::sigmoid_impl(_mm_or_ps(pre, tmp));
}
inline __m128 sigmoid_pre(float a, float b) noexcept {
	float result[4];
	result[0] = 0.0f;
	result[1] = std::exp(a*b);
	result[2] = std::exp(b*(a - 1));
	result[3] = result[1];

	return _mm_loadu_ps(result);
}
