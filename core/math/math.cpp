#include "math.hpp"

#include <cmath>
#include <random>
#include <algorithm>

extern "C" {

double hbpl_math_abs(double x) {
    return std::abs(x);
}

double hbpl_math_sqrt(double x) {
    return std::sqrt(x);
}

double hbpl_math_pow(double x, double y) {
    return std::pow(x, y);
}

double hbpl_math_sin(double x) {
    return std::sin(x);
}

double hbpl_math_cos(double x) {
    return std::cos(x);
}

double hbpl_math_tan(double x) {
    return std::tan(x);
}

double hbpl_math_asin(double x) {
    return std::asin(x);
}

double hbpl_math_acos(double x) {
    return std::acos(x);
}

double hbpl_math_atan(double x) {
    return std::atan(x);
}

double hbpl_math_floor(double x) {
    return std::floor(x);
}

double hbpl_math_ceil(double x) {
    return std::ceil(x);
}

double hbpl_math_round(double x) {
    return std::round(x);
}

double hbpl_math_min(double a, double b) {
    return std::min(a, b);
}

double hbpl_math_max(double a, double b) {
    return std::max(a, b);
}

double hbpl_math_clamp(double x, double min, double max) {
    return std::clamp(x, min, max);
}

double hbpl_math_log(double x) {
    return std::log(x);
}

double hbpl_math_log10(double x) {
    return std::log10(x);
}

double hbpl_math_exp(double x) {
    return std::exp(x);
}

double hbpl_math_pi() {
    return 3.14159265358979323846;
}

double hbpl_math_e() {
    return 2.71828182845904523536;
}

double hbpl_math_random() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<double> dist(0.0, 1.0);

    return dist(gen);
}

double hbpl_math_random_range(double min, double max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_real_distribution<double> dist(min, max);

    return dist(gen);
}

}
