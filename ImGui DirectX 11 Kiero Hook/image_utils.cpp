#include "image_utils.h"

void fill_outline(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& out)
{
	const int max_epochs = config->FILL_ITERATIONS;
	const int pixelbox_size = config->PIXELBOX_SIZE;
	const int start_x = NEURAL_NETWORK_INPUT_X / 2 - (pixelbox_size * 2);
	const int start_y = NEURAL_NETWORK_INPUT_Y / 2 - (pixelbox_size * 2);
	for (int i = 0; i < max_epochs; i++)
	{
		for (int x = start_x; x < start_x + pixelbox_size * 4; x++)
		{
			for (int y = start_y; y < start_y + pixelbox_size * 4; y++)
			{
				int pixel_count = 0;
				if (pixels[x][y] == 0xFFFFFFFF)
				{
					out[x][y] = 0xFFFFFFFF;
					continue;
				}
				if (pixels[x + 1][y] == 0xFFFFFFFF)
				{
					pixel_count++;
				}
				if (pixels[x - 1][y] == 0xFFFFFFFF)
				{
					pixel_count++;
				}
				if (pixels[x][y + 1] == 0xFFFFFFFF)
				{
					pixel_count++;
				}
				if (pixels[x][y - 1] == 0xFFFFFFFF)
				{
					pixel_count++;
				}
				if (pixels[x + 1][y + 1] == 0xFFFFFFFF)
				{
					pixel_count++;
				}
				if (pixels[x - 1][y - 1] == 0xFFFFFFFF)
				{
					pixel_count++;
				}
				if (pixels[x + 1][y - 1] == 0xFFFFFFFF)
				{
					pixel_count++;
				}
				if (pixels[x - 1][y + 1] == 0xFFFFFFFF)
				{
					pixel_count++;
				}
				if (pixel_count > 3)
				{
					out[x][y] = 0xFFFFFFFF;
				}
				else out[x][y] = 0xFF000000;
			}
		}
		std::copy(out.begin(), out.end(), pixels.begin());
	}
}

cv::Mat convert_pixelarray_to_vec(UINT pixels[NEURAL_NETWORK_INPUT_X][NEURAL_NETWORK_INPUT_Y])
{
	return cv::Mat(NEURAL_NETWORK_INPUT_X, NEURAL_NETWORK_INPUT_Y, CV_8UC4, pixels);
}

cv::Mat convert_to_mat_no_alpha(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels)
{
	cv::Mat mat = cv::Mat(NEURAL_NETWORK_INPUT_X, NEURAL_NETWORK_INPUT_Y, CV_8UC4, pixels.data());
	cv::Mat mat_no_alpha;
	cv::cvtColor(mat, mat_no_alpha, cv::COLOR_RGBA2RGB);
	return mat_no_alpha;
}

void convert_mat_to_pixelarray(cv::Mat mat, std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels)
{
	cv::Mat mat_rgba;
	cv::cvtColor(mat, mat_rgba, cv::COLOR_RGB2RGBA);
	memcpy(&pixels[0][0], mat_rgba.data, NEURAL_NETWORK_INPUT_X * NEURAL_NETWORK_INPUT_Y * 4);
}

cv::Mat convert_to_binary(cv::Mat mat)
{
	cv::Mat img_in;
	cv::cvtColor(mat, img_in, cv::COLOR_RGB2GRAY);
	cv::Mat img_th;
	cv::threshold(img_in, img_th, 253, 257, cv::THRESH_BINARY);
	return img_th;
}

void draw_detections(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, std::vector<Track> detections)
{
	cv::Mat no_alpha = convert_to_mat_no_alpha(pixels);
	std::string text;

	for (int i = 0; i < detections.size(); i++)
	{
		auto detection = gsl::at(detections, i);

		switch (detection->getClassId())
		{
		case 0:
			cv::rectangle(no_alpha, cv::Rect(detection->getRect().x(), detection->getRect().y(), detection->getRect().width(), detection->getRect().height()), cv::Scalar(255, 0, 0), 1);
			text = "Body" + std::to_string(detection->getTrackId());
			cv::putText(no_alpha, text, cv::Point(detection->getRect().x(), detection->getRect().y()), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1);
			convert_mat_to_pixelarray(no_alpha, pixels);
			break;
		case 1:
			cv::rectangle(no_alpha, cv::Rect(detection->getRect().x(), detection->getRect().y(), detection->getRect().width(), detection->getRect().height()), cv::Scalar(0, 255, 0), 1);
			text = "Head" + std::to_string(detection->getTrackId());
			cv::putText(no_alpha, text, cv::Point(detection->getRect().x(), detection->getRect().y()), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
			convert_mat_to_pixelarray(no_alpha, pixels);
			break;
		case 2:
			cv::rectangle(no_alpha, cv::Rect(detection->getRect().x(), detection->getRect().y(), detection->getRect().width(), detection->getRect().height()), cv::Scalar(0, 0, 255), 1);
			text = "Iso Ball" + std::to_string(detection->getTrackId());
			cv::putText(no_alpha, text, cv::Point(detection->getRect().x(), detection->getRect().y()), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
			convert_mat_to_pixelarray(no_alpha, pixels);
			break;
		case 3:
			cv::rectangle(no_alpha, cv::Rect(detection->getRect().x(), detection->getRect().y(), detection->getRect().width(), detection->getRect().height()), cv::Scalar(255, 255, 0), 1);
			text = "Ball" + std::to_string(detection->getTrackId());;
			cv::putText(no_alpha, text, cv::Point(detection->getRect().x(), detection->getRect().y()), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
			convert_mat_to_pixelarray(no_alpha, pixels);
			break;
		default:
			break;
		}
	}
}

void draw_aimbot(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels)
{
	const float fov = config->AIMBOT_FOV;
	const float fov_pixels_X = target_detector->DegreesToPixelsX(fov) / 2;

	cv::Mat no_alpha = convert_to_mat_no_alpha(pixels);

	cv::ellipse(no_alpha, cv::Point(NEURAL_NETWORK_INPUT_X / 2, NEURAL_NETWORK_INPUT_Y / 2), cv::Size(fov_pixels_X, fov_pixels_X), 0, 0, 360, cv::Scalar(0, 255, 0), 1);

	convert_mat_to_pixelarray(no_alpha, pixels);
}

void draw_aim_assist(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels)
{
	const float fov = config->AIM_ASSIST_FOV;
	const float fov_pixels_X = target_detector->DegreesToPixelsX(fov) / 2;

	cv::Mat no_alpha = convert_to_mat_no_alpha(pixels);

	cv::ellipse(no_alpha, cv::Point(NEURAL_NETWORK_INPUT_X / 2, NEURAL_NETWORK_INPUT_Y / 2), cv::Size(fov_pixels_X, fov_pixels_X), 0, 0, 360, cv::Scalar(255, 0, 0), 1);

	convert_mat_to_pixelarray(no_alpha, pixels);
}

void draw_aimbot_target(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, Target target)
{
	cv::Mat no_alpha = convert_to_mat_no_alpha(pixels);

	cv::rectangle(no_alpha, cv::Rect(target.x_pixels + NEURAL_NETWORK_INPUT_X / 2, target.y_pixels + NEURAL_NETWORK_INPUT_Y / 2, 5, 5), cv::Scalar(102, 51, 153), 1);

	convert_mat_to_pixelarray(no_alpha, pixels);
}

void draw_aim_assist_target(std::array<std::array<UINT, NEURAL_NETWORK_INPUT_Y>, NEURAL_NETWORK_INPUT_X>& pixels, Target target)
{
	cv::Mat no_alpha = convert_to_mat_no_alpha(pixels);

	cv::rectangle(no_alpha, cv::Rect(target.x_pixels + NEURAL_NETWORK_INPUT_X / 2, target.y_pixels + NEURAL_NETWORK_INPUT_Y / 2, 5, 5), cv::Scalar(255, 0, 255), 1);

	convert_mat_to_pixelarray(no_alpha, pixels);
}

