#pragma once

#include <math.h>

#include <common.h>



namespace nc {

	inline float lerp(float a, float b, float t) {
		return a + ((b - a) * t);
	}

	inline double lerp(double a, double b, float t) {
		return a + ((b - a) * t);
	}


	template<typename T = float>
	struct easings 
	{
	private:

		static constexpr T Pi = (T)3.14159265358979323846;
		static constexpr T PiHalf = (T)(Pi * 0.5);

		static inline bool is_in_01(const T t) { return ((T)0.0) <= t && t <= ((T)1.0); }
		static inline T sqr(const T t)  { return (t * t); }
		static inline T pow3(const T t) { return (t * t) * t; }
		static inline T pow4(const T t) { return (t * t) * (t * t); }
		static inline T pow5(const T t) { return (t * t) * (t * t) * t; }

	public:


		using easing_func_t = T(T t);


		static inline T linear(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)t;
		}


		static inline T sine_in(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)sin(PiHalf * t);
		}

		static inline T sine_out(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)(1.0 + sin(PiHalf * (t - 1.0)));
		}

		static inline T sine_in_out(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)(0.5 * (1.0 + sin(Pi * (t - 0.5))));
		}


		static inline T quadratic_in(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)sqr(t);
		}

		static inline T quadratic_out(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)(1.0 - sqr(1.0 - t));
		}

		static inline T quadratic_in_out(const T t)
		{
			nc_assert(is_in_01(t));

			if (t < 0.5) {
				return (T)(2.0 * sqr(t));
			}
			else {
				return (T)((t * (4.0 - (2.0 * t))) - 1.0);
			}
		}


		static inline T cubic_in(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)pow3(t);
		}

		static inline T cubic_out(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)(1.0 - pow3(1.0 - t));
		}

		static inline T cubic_in_out(const T t)
		{
			nc_assert(is_in_01(t));

			if (t < 0.5) {
				return (T)(4.0 * pow3(t));
			}
			else {
				return (T)(1.0 - (pow3(-2.0 * t + 2.0) / 2.0));
			}
		}


		static inline T quart_in(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)pow4(t);
		}

		static inline T quart_out(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)(1.0 - pow4(1.0 - t));
		}

		static inline T quart_in_out(const T t)
		{
			nc_assert(is_in_01(t));

			if (t < 0.5) {
				return (T)(8.0 * pow4(t));
			}
			else {
				return (T)(1.0 - (pow4(-2.0 * t + 2.0) / 2.0));
			}
		}


		static inline T quint_in(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)pow5(t);
		}

		static inline T quint_out(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)(1.0 - pow5(1.0 - t));
		}

		static inline T quint_in_out(const T t)
		{
			nc_assert(is_in_01(t));

			if (t < 0.5) {
				return (T)(16.0 * pow5(t));
			}
			else {
				return (T)(1.0 - (pow5(-2.0 * t + 2.0) / 2.0));
			}
		}


		static inline T circ_in(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)(1.0 - sqrt(1.0 - t));
		}

		static inline T circ_out(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)sqrt(t);
		}

		static inline T circ_in_out(const T t)
		{
			nc_assert(is_in_01(t));

			if (t < 0.5) {
				return (T)((1.0 - sqrt(1.0 - 2.0 * t)) * 0.5);
			}
			else {
				return (T)((1.0 + sqrt(2.0 * t - 1.0)) * 0.5);
			}
		}


		static inline T bounce_in(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)(exp2(6.0 * (t - 1.0)) * abs(sin(t * Pi * 3.5)));
		}

		static inline T bounce_out(const T t)
		{
			nc_assert(is_in_01(t));

			return (T)(1.0 - exp2(-6.0 * t) * abs(cos(t * Pi * 3.5)));
		}

		static inline T bounce_in_out(const T t)
		{
			nc_assert(is_in_01(t));

			if (t < 0.5) {
				return (T)(8.0 * exp2(8.0 * (t - 1.0)) * abs(sin(t * Pi * 7.0)));
			}
			else {
				return (T)(1.0 - 8.0 * exp2(-8.0 * t) * abs(sin(t * Pi * 7.0)));
			}
		}

	};


	using easingsf = easings<float>;
	using easingsd = easings<double>;
}