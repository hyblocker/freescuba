#include "one_euro_filter.hpp"
#include <cmath>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MATH_PI       3.14159265358979323846

// Returns instantaneous system time in seconds
double GetSystemTimeSeconds() {

    LARGE_INTEGER t1 = {};
    LARGE_INTEGER freq = {};

    // High resolution windows only clocks
    QueryPerformanceCounter(&t1);
    QueryPerformanceFrequency(&freq);

    return static_cast<double>(t1.QuadPart) / static_cast<double>(freq.QuadPart);
}

OneEuroFilter::OneEuroFilter(
    double x0,
    double dx0,
    double min_cutoff,
    double beta,
    double d_cutoff)
      
    :   // The parameters.
        min_cutoff(min_cutoff),
        beta(beta),
        d_cutoff(d_cutoff),
        // Previous values.
        x_prev(x0),
        dx_prev(dx0),
        t_prev(GetSystemTimeSeconds())
{}

double OneEuroFilter::SmoothingFactor(double t_e, double cutoff) {
    double r = 2.0 * MATH_PI * cutoff * t_e;
    return r / (r + 1.0);
}

double OneEuroFilter::ExponentialSmoothing(double a, double x, double x_prev) {
    return a * x + (1.0 - a) * x_prev;
}

double OneEuroFilter::Call(double x) {

    double t = GetSystemTimeSeconds();

    double t_e = t - t_prev;

    // The filtered derivative of the signal.
    double a_d      = SmoothingFactor(t_e, d_cutoff);
    double dx       = (x - x_prev) / t_e;
    double dx_hat   = ExponentialSmoothing(a_d, dx, dx_prev);

    // The filtered signal.
    double cutoff   = min_cutoff + beta * abs(dx_hat);
    double a        = SmoothingFactor(t_e, cutoff);
    double x_hat    = ExponentialSmoothing(a, x, x_prev);

    // Memorize the previous values.
    x_prev          = x_hat;
    dx_prev         = dx_hat;
    t_prev          = t;

    return x_hat;
}