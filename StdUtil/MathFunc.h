#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include <float.h>
#include <vector>

namespace MathFunc2 {

	constexpr double TWO_PI = M_PI * 2.0;
	constexpr double PI_OVER_2 = M_PI / 2.0;

	constexpr double DEG2RAD = M_PI / 180.0;
	constexpr double RAD2DEG = 180.0 / M_PI;
	constexpr double RAD2GRAD = 200.0 / M_PI;
	constexpr double GRAD2RAD = M_PI / 200.0;
	constexpr double GRAD2DEG = 180.0 / 200.0;
	constexpr double DEG2GRAD = 200.0 / 180.0;

	constexpr double M2FEET = 3.280841;
	constexpr double FEET2M = 0.3048;

	// Convert degrees to radians
	inline double degToRad(double angle)
	{
		return angle * DEG2RAD;
	}
	// Convert radians to degrees
	inline double radToDeg(double angle)
	{
		return angle * RAD2DEG;
	}
	// Convert radians to grads
	inline double radToGrad(double angle)
	{
		return angle * RAD2GRAD;
	}
	// Covert grads to radians
	inline double gradToRad(double angle)
	{
		return angle * GRAD2RAD;
	}
	inline double gradToDeg(double angle)
	{
		return angle * GRAD2DEG;
	}
	inline double degToGrad(double angle)
	{
		return angle * DEG2GRAD;
	}

	// round to integer
	inline int iround(double x)
	{
		return (int)floor(x + 0.5);
	}

	// round to long
	inline long lround(double x)
	{
		return (long)floor(x + 0.5);
	}

	// round number n to d decimal points

	inline double fround(double n, unsigned d)
	{
		return floor(n * pow(10.0, (int)(d)) + 0.5) / pow(10.0, (int)(d));
	}

	// finds the closest whole multiple of "mult" to "value"
	inline double closestMultipleD(double value, double mult)
	{
		return mult * floor((value / mult) + 0.5);
	}

	inline int closestMultipleI(int n, int mult)
	{
		if (mult > n)
			return mult;

		n = n + mult / 2;
		n = n - (n % mult);
		return n;
	}

	inline double acos_safe(double val)
	{
		if (val > 1.0) return 0.0;
		if (val < -1.0) return acos(-1.0);
		return acos(val);
	}

	inline double asin_safe(double val)
	{
		if (val > 1.0) return asin(1.0);
		if (val < -1.0) return asin(-1.0);
		return asin(val);
	}

	inline double logBase2(double inValue)
	{
		return _logb(inValue) / _logb(2.0);

	}

	inline double logToBase(double value, double base)
	{
		return _logb(value) / _logb(base);

	}
}
