#include "yolo.h"

InferenceEngine inference = InferenceEngine();

void process_frame(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels)
{
	cv::Mat frame = convert_to_mat_no_alpha(pixels);

	std::vector<float> input_tensor_values = inference.preprocessImage(frame);
	std::vector<float> results = inference.runInference(input_tensor_values);

	const auto input_shape = inference.input_shape;
	std::vector<Detection> detections = inference.filterDetections(results, gsl::at(input_shape, 2), gsl::at(input_shape, 3), NEURAL_NETWORK_INPUT_X, NEURAL_NETWORK_INPUT_Y);
	
	tracker->update(detections);
	target_detector->find_closest_target();
}

//void draw_rect_no_fill(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a, int line_width)
//{
//	if (line_width > w || line_width > h) return;
//	for (int i = x; i < x + w; i++)
//	{
//		if (i < 0 || i > NEURAL_NETWORK_INPUT_X - 1) continue;
//		for (int j = y; j < y + line_width; j++)
//		{
//			if (j < 0 || j > NEURAL_NETWORK_INPUT_Y - 1) continue;
//			pixels[j][i] = a << 24 | r | g << 8 | b << 16;
//		}
//	}
//	for (int i = x; i < x + w; i++)
//	{
//		if (i < 0 || i > NEURAL_NETWORK_INPUT_X - 1) continue;
//		for (int j = y + h - line_width; j < y + h; j++)
//		{
//			if (j < 0 || j > NEURAL_NETWORK_INPUT_Y - 1) continue;
//			pixels[j][i] = a << 24 | r | g << 8 | b << 16;
//		}
//	}
//	for (int i = x; i < x + line_width; i++)
//	{
//		if (i < 0 || i > NEURAL_NETWORK_INPUT_X - 1) continue;
//		for (int j = y; j < y + h; j++)
//		{
//			if (j < 0 || j > NEURAL_NETWORK_INPUT_Y - 1) continue;
//			pixels[j][i] = a << 24 | r | g << 8 | b << 16;
//		}
//	}
//	for (int i = x + w - line_width; i < x + w; i++)
//	{
//		if (i < 0 || i > NEURAL_NETWORK_INPUT_X - 1) continue;
//		for (int j = y; j < y + h; j++)
//		{
//			if (j < 0 || j > NEURAL_NETWORK_INPUT_Y - 1) continue;
//			pixels[j][i] = a << 24 | r | g << 8 | b << 16;
//		}
//	}
//}

//void draw_dot(UINT pixels[NEURAL_NETWORK_INPUT_X][NEURAL_NETWORK_INPUT_Y], int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a, int size)
//{
//	int middle_x = static_cast<int>(x + w * 0.5f);
//	int middle_y = static_cast<int>(y + h * 0.1f);
//
//	for (int i = middle_x - size / 2; i < middle_x + size / 2; i++)
//	{
//		if (i < 0 || i > NEURAL_NETWORK_INPUT_X - 1) continue;
//		for (int j = middle_y - size / 2; j < middle_y + size / 2; j++)
//		{
//			if (j < 0 || j > NEURAL_NETWORK_INPUT_Y - 1) continue;
//			pixels[j][i] = a << 24 | r | g << 8 | b << 16;
//		}
//	}
//}

float get_confidence_of_index(int index)
{
	std::vector<Track> detections = tracker->tracks;
	if (detections.size() == 0)
		return 0.0f;
	return gsl::at(detections, index)->getScore();
}

std::pair<int, int> get_xy_of_index(int index)
{
	std::vector<Track> detections = tracker->tracks;
	if (detections.size() == 0) return std::make_pair(0, 0);
	return std::make_pair(gsl::at(detections, index)->getRect().x(), gsl::at(detections, index)->getRect().y());
}

std::pair<int, int> get_wh_of_index(int index)
{
	std::vector<Track> detections = tracker->tracks;
	if (detections.size() == 0) return std::make_pair(0, 0);
	return std::make_pair(gsl::at(detections, index)->getRect().width(), gsl::at(detections, index)->getRect().height());
}