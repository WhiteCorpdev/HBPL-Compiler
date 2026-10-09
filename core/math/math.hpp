#pragma once

extern "C" {

double hbpl_math_abs(double x);
double hbpl_math_sqrt(double x);
double hbpl_math_pow(double x, double y);

double hbpl_math_sin(double x);
double hbpl_math_cos(double x);
double hbpl_math_tan(double x);

double hbpl_math_asin(double x);
double hbpl_math_acos(double x);
double hbpl_math_atan(double x);

double hbpl_math_floor(double x);
double hbpl_math_ceil(double x);
double hbpl_math_round(double x);

double hbpl_math_min(double a, double b);
double hbpl_math_max(double a, double b);
double hbpl_math_clamp(double x, double min, double max);

double hbpl_math_log(double x);
double hbpl_math_log10(double x);
double hbpl_math_exp(double x);

double hbpl_math_pi();
double hbpl_math_e();

double hbpl_math_random();
double hbpl_math_random_range(double min, double max);

}
