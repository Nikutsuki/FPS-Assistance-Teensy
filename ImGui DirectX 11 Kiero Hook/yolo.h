#pragma once

#include "inference.h"
#include "Globals.h"
#include "image_utils.h"

void process_frame(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>&);
cv::Mat convert_pixelarray_to_vec(UINT pixels[NEURAL_NETWORK_INPUT_X][NEURAL_NETWORK_INPUT_Y]);
void draw_rect_no_fill(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a, int line_width);
void draw_dot(UINT pixels[NEURAL_NETWORK_INPUT_X][NEURAL_NETWORK_INPUT_Y], int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a, int size);
float get_confidence_of_index(int index);
std::pair<int, int> get_xy_of_index(int index);
std::pair<int, int> get_wh_of_index(int index);
extern InferenceEngine inference;