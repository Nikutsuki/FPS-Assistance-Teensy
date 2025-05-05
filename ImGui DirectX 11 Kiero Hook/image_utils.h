#pragma once
#include "Globals.h"
#include "includes.h"

void fill_outline(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& out);
cv::Mat convert_pixelarray_to_vec(UINT pixels[NEURAL_NETWORK_INPUT_X][NEURAL_NETWORK_INPUT_Y]);
cv::Mat convert_to_mat_no_alpha(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels);
void convert_mat_to_pixelarray(cv::Mat mat, std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels);
cv::Mat convert_to_binary(cv::Mat mat);
void draw_fov(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, float fov, UINT color);
void draw_detections(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, std::vector<Track> detections);
void draw_aimbot(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels);
void draw_aim_assist(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels);
void draw_aimbot_target(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, Target target);
void draw_aim_assist_target(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, Target target);