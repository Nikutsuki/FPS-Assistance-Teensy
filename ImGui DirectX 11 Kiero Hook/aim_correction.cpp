#include "aim_correction.h"

static float ratio_x(float x) noexcept
{
	return x / (static_cast<float>(NEURAL_NETWORK_INPUT_X) / 2);
}

static float ratio_y(float y) noexcept
{
	return y / (static_cast<float>(NEURAL_NETWORK_INPUT_Y) / 2);
}

static float linear(float x) noexcept
{
	return std::min(1.f / (1.f + exp(-15.f * abs(x) + 2.f)) + 0.61f, 1.f);
}

void aim_correction(int& mouse_x, int& mouse_y, Target target) noexcept
{
	mouse_x = static_cast<int>(mouse_x * linear(ratio_x(target.x_pixels)));
	mouse_y = static_cast<int>(mouse_y * linear(ratio_y(target.y_pixels)));
}