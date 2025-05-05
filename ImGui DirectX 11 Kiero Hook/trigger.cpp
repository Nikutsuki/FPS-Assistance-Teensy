#include "trigger.h"

bool trigger_check(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels) noexcept
{
	const int pixelbox_size = config->PIXELBOX_SIZE;
	const int start_x = NEURAL_NETWORK_INPUT_X / 2 - pixelbox_size;
	const int start_y = NEURAL_NETWORK_INPUT_Y / 2 - pixelbox_size;

	for (int x = start_x; x < start_x + pixelbox_size * 2; x++)
	{
		for (int y = start_y; y < start_y + pixelbox_size * 2; y++)
		{
			const unsigned char r = pixels[y][x] & 0xFF;
			const unsigned char g = (pixels[y][x] >> 8) & 0xFF;
			const unsigned char b = (pixels[y][x] >> 16) & 0xFF;

			if (filter(r, g, b))
			{
				return true;
			}
		}
	}
	return false;
}

bool filter(unsigned char r, unsigned char g, unsigned char b) noexcept
{
	hsv color = rgb2hsv(r, g, b);
	if (color.h >= config->HUE_MIN && color.h <= config->HUE_MAX && color.s >= config->SATURATION_MIN && color.s <= config->SATURATION_MAX && color.v >= config->VALUE_MIN && color.v <= config->VALUE_MAX)
	{
		return true;
	}
	return false;
}

hsv rgb2hsv(unsigned char r, unsigned char g, unsigned char b) noexcept
{
	hsv hsv{};
	const float r_ = r / 255.0f;
	const float g_ = g / 255.0f;
	const float b_ = b / 255.0f;

	const float max = std::max(r_, std::max(g_, b_));
	const float min = std::min(r_, std::min(g_, b_));

	if (max == min)
	{
		hsv.h = 0;
	}
	else if (max == r_)
	{
		hsv.h = 60.0f * (g_ - b_) / (max - min);
	}
	else if (max == g_)
	{
		hsv.h = 60.0f * (2 + (b_ - r_) / (max - min));
	}
	else if (max == b_)
	{
		hsv.h = 60.0f * (4 + (r_ - g_) / (max - min));
	}

	if (hsv.h < 0)
	{
		hsv.h += 360.0f;
	}

	if (max == 0)
	{
		hsv.s = 0;
	}
	else
	{
		hsv.s = (max - min) / max;
	}

	hsv.v = max;

	return hsv;
}
