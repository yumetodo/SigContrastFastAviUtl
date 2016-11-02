#pragma once
#include <cmath>
#include <xmmintrin.h>
inline __m128 m128_rcp_22bit_ps(const __m128& a) noexcept {
	const __m128 xm1 = _mm_rcp_ps(a);//(1/a):11bit
	const __m128 xm0 = _mm_mul_ps(_mm_mul_ps(a, xm1), xm1);//a * (1/a) * (1/a)
	return _mm_sub_ps(_mm_add_ps(xm1, xm1), xm0);//(1/a) + (1/a) - a * (1/a) * (1/a)
}
inline float sigmoid(float a, float b, float u) noexcept {
	//float e1, e2, e3, e4;
	float result[4];
	result[0] = std::expf(b*(a - u));
	result[1] = std::expf(a*b);
	result[2] = std::expf(b*(a - 1));
	result[3] = result[1];

	__m128 ei = _mm_loadu_ps(result);
	static const __m128 v1 = _mm_set1_ps(1.0f);
	const __m128 dst = _mm_add_ps(ei, v1);
	ei = m128_rcp_22bit_ps(dst);

	_mm_storeu_ps(result, ei);
	return (result[0] - result[1]) / (result[2] - result[3]);
}
