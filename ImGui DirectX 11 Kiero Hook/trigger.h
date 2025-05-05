#pragma once
#include "includes.h"
#include "Globals.h"

bool trigger_check(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels) noexcept;
bool filter(unsigned char r, unsigned char g, unsigned char b) noexcept;
struct hsv {
	float h;
	float s;
	float v;
};

hsv rgb2hsv(unsigned char r, unsigned char g, unsigned char b) noexcept;