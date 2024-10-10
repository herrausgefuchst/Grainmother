#pragma once

#define BELA_CONNECTED

#include <iostream>
#include <vector>
#include <math.h>
#include <ctime>
#include <array>
#include <functional>
#include <string>
#include <memory>
#include <algorithm>
#include <cmath>
#include <thread>
#include <mutex>
#include <numeric>
#include <cstring>
#include <arm_neon.h>

#ifdef BELA_CONNECTED

#include <libraries/math_neon/math_neon.h>

#else

#define tanf_neon(x) tanf(x)
#define sqrtf_neon(x) sqrtf(x)
#define powf_neon(x, y) powf(x, y)
#define sinhf_neon(x) sinhf(x)
#define sinf_neon(x) sinf(x)
#define cosf_neon(x) cosf(x)
#define logf_neon(x) logf(x)
#define cosf_neon(x) cosf(x)
#define floorf_neon(x) floorf(x)
#define ceilf_neon(x) ceilf(x)
#define fabsf_neon(x) fabsf(x)
#define fmodf_neon(x, y) fmodf(x, y)

#define rt_printf(...) printf(__VA_ARGS__)

#endif

using String = std::string; ///< Alias for the standard string type.

static const float PI = 3.14159265358979323846f; ///< The mathematical constant π, approximately 3.14159.
static const float TWOPI = 2.f * PI; ///< Two times the mathematical constant π, approximately 6.28319.
static const float TWOoPI = 2.f / PI; ///< Two divided by the mathematical constant π, approximately 0.63662.
static const float PIo2 = PI / 2.f; ///< The mathematical constant π divided by 2, approximately 1.57080.
static const float PI3o2 = 3.f * PI / 2.f; ///< Three times the mathematical constant π divided by 2, approximately 4.71239.

static const float sqrt_2 = sqrtf(2.f); ///< The square root of 2, approximately 1.41421.
static const float log_2 = logf(2.f); ///< The natural logarithm of 2, approximately 0.69315.

static const float SMALLEST_POSITIVE_FLOATVALUE = 1.17549e-38; ///< The smallest positive representable float value, approximately 1.17549e-38.
static const float SMALLEST_NEGATIVE_FLOATVALUE = -1.17549e-38; ///< The smallest negative representable float value, approximately -1.17549e-38.

static const float RAND_MAX_INVERSED = (float)(1.f / RAND_MAX);
