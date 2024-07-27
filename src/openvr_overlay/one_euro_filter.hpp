#pragma once

class OneEuroFilter {

public:

	/// <summary>
	/// Initialize the one euro filter.
	/// </summary>
	OneEuroFilter(
        double x0,
        double dx0 = 0.0,
        double min_cutoff = 1.0,
        double beta = 0.0,
        double d_cutoff = 1.0);

    /// <summary>
    /// Compute the filtered signal.
    /// </summary>
    double Call(double x);

private:
    double SmoothingFactor(double t_e, double cutoff);
    double ExponentialSmoothing(double a, double x, double x_prev);

private:
    double min_cutoff;
    double beta;
    double d_cutoff;
    double x_prev;
    double dx_prev;
    double t_prev;
};