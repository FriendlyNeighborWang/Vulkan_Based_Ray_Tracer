#ifndef PBRT_UTIL_MATH_H
#define PBRT_UTIL_MATH_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cmath>

#include "base/base.h"
#include "util/pstd.h"

namespace pstd {
	
	constexpr double Pi = 3.14159265358979323846;
	constexpr double InvPi = 0.31830988618379067154;
	constexpr double Inv2Pi = 0.15915494309189533577;
	constexpr double Inv4Pi = 0.07957747154594766788;
	constexpr double PiDiv2 = 1.57079632679489661923;
	constexpr double PiDiv4 = 0.78539816339744830961;
	constexpr double Sqrt2 = 1.41421356237309504880;
	constexpr double Sigma = 1e-7f;
	constexpr double Infinity = std::numeric_limits<double>::infinity();


	inline double Lerp(double x, double a, double b) {
		return (1 - x) * a + x * b;
	}

	inline double FMA(double a, double b, double c) {
		return std::fma(a, b, c);
	}
	
	inline double SinxDivX(double x) {
		if (1 - x * x == 1)
			return 1;
		return std::sin(x) / x;
	}

	inline double Sinc(double x) {
		return SinxDivX(Pi * x);
	}

	inline double WindowedSinc(double x, double radius, double tau) {
		if (pstd::abs(x) > radius)
			return 0;
		return Sinc(x) * Sinc(x / tau);
	}

	template<typename T, typename U, typename V>
	inline constexpr T Clamp(T val, U low, V high) {
		if (val < low)
			return T(low);
		else if (val > high)
			return T(high);
		else 
			return val;
	}

	template <typename T>
	inline T Mod(T a, T b) {
		T result = a - (a / b) * b;
		return static_cast<T>((result < 0) ? result + b : result);
	}

	template<>
	inline double Mod(double a, double b) {
		return std::fmod(a, b);
	}

	inline double Radians(double degrees) {
		return (degrees / 180.0) * Pi;
	}

	inline double Degrees(double radians) {
		return (radians / Pi) * 180.0;
	}

	inline double SmoothStep(double x, double a, double b) {
		if (a == b)
			return (x < a) ? 0 : 1;
		assert(a < b && "SmoothStep: a must be less than b");
		double t = Clamp((x - a) / (b - a), 0, 1);
		return t * t * (3 - 2 * t);
	}

	inline double SafeSqrt(double x) {
		assert((x >= -1e-3) && "SafeSqrt: x must be greater than 0");
		return pstd::sqrt(std::max(0., double(x)));
	}

	inline double SafeASin(double x) {
		assert(x >= -1.0001 && x <= 1.0001 && "SafeAsin: x must be between -1 and 1 ");
		return std::asin(Clamp(x, -1, 1));
	}

	inline double SafeACos(double x) {
		assert(x >= -1.0001 && x <= 1.0001 && "SafeAcos: x must be between -1 and 1 ");
		return std::acos(Clamp(x, -1, 1));
	}


	template <typename T>
	inline constexpr T Sqr(T v) {
		return v * v;
	}

	template<int n>
	inline constexpr double Pow(double v) {
		if constexpr (n < 0)
			return 1 / Pow<-n>(v);
		float n2 = Pow<n / 2>(v);
		return n2 * n2 * Pow<n & 1>(v);
	}

	template<>
	inline constexpr double Pow<1>(double v) {
		return v;
	}

	template<>
	inline constexpr double Pow<0>(double v) {
		return 1;
	}

	template<typename F, typename C>
	inline constexpr double EvaluatePolynormial(double t, C c) {
		return c;
	}

	template<typename F, typename C, typename... Args>
	inline constexpr double EvaluatePolynormial(double t, C c, Args... Remaining) {
		return FMA(t, EvaluatePolynormial<F>(t, Remaining...), c);
	}


}


#endif // !PBRT_UTIL_MATH_H
