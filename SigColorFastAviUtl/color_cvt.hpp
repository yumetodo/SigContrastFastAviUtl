#pragma once
#include "filter.h"
#include <xmmintrin.h>
namespace color_cvt {
	inline void yc2rgb(float(&buf)[4], const PIXEL_YC* px) noexcept {
		// Load YUV2RGB matrix
		const __m128 cy = _mm_set_ps(1.0037736040867458f,     1.0031713814217937f,  1.0038646965904563f,    0.f);
		const __m128 cu = _mm_set_ps(0.0009812686948862392f, -0.34182057237626395f, 1.7738420513779833f,    0.f);
		const __m128 cv = _mm_set_ps(1.4028706125758748f,    -0.7126004638855613f,  0.0018494308641594699f, 0.f);
		__m128 my = _mm_set1_ps(static_cast<float>(px->y));
		__m128 mu = _mm_set1_ps(static_cast<float>(px->cb));
		__m128 mv = _mm_set1_ps(static_cast<float>(px->cr));

		my = _mm_mul_ps(my, cy);
		mu = _mm_mul_ps(mu, cu);
		mv = _mm_mul_ps(mv, cv);

		my = _mm_add_ps(my, mu);
		my = _mm_add_ps(my, mv); //result in my

		_mm_storeu_ps(buf, my); // buf: 0, b, g, r
	}
	inline void rgb2yc(PIXEL_YC* px, const float(&buf)[4]) noexcept {
		// Load RGB2YUV matrix
		const __m128 cr = _mm_set_ps(0.297607421875f, -0.1689453125f,    0.5f,            0.f);
		const __m128 cg = _mm_set_ps(0.586181640625f, -0.331298828125f, -0.419189453125f, 0.f);
		const __m128 cb = _mm_set_ps(0.11279296875f,   0.5f,            -0.0810546875f,   0.f);
		__m128 my = _mm_set1_ps(buf[1]);
		__m128 mu = _mm_set1_ps(buf[2]);
		__m128 mv = _mm_set1_ps(buf[3]);
		my = _mm_mul_ps(my, cb);
		mu = _mm_mul_ps(mu, cg);
		mv = _mm_mul_ps(mv, cr);
		my = _mm_add_ps(my, mu);
		my = _mm_add_ps(my, mv); //result in my

		float tmp[4];
		_mm_storeu_ps(tmp, my); // tmp: 0, v, u, y
		px->y = static_cast<short>(tmp[3]);
		px->cb = static_cast<short>(tmp[2]);
		px->cr = static_cast<short>(tmp[1]);
	}
}
