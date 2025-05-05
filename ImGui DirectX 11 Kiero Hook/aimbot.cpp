#include "aimbot.h"

static float ratio_x(float x) noexcept
{
	return x / (static_cast<float>(NEURAL_NETWORK_INPUT_X) / 2.f);
}

static float ratio_y(float y) noexcept
{
	return y / (static_cast<float>(NEURAL_NETWORK_INPUT_Y) / 2.f);
}

static float sigmoid(float x) noexcept
{
	return (1.0f / (1.0f + exp(-10.0f * x + 5.0f))) + (x / 2.0f);
}

static float sqrt_func(float x) noexcept
{
	if (x == 0)
	{
		return 0;
	}
	if (x > 0)
	{
		return sqrt(x);
	}
	if (x < 0)
	{
		return -sqrt(-x);
	}
	return 0;
}

static float log_func(float x) noexcept
{
	return -log(-x + 1.2f) + 0.3f;
}

static float linear(float x) noexcept
{
	return x / 300;
}

static float x_over_x(float x) noexcept
{
	return x / (1 + x);
}

//void Aimbot::loop()
//{
//	while (true)
//	{
//		if (!config->AIMBOT_ON || config->AIMBOT_TIME == 0) continue;
//
//		std::vector<Target> targets = *target_detector->targets;
//
//		if (targets.size() == 0) continue;
//
//		select_func();
//
//		for (auto& target : targets)
//		{
//			if (abs(target.x_pixels) <= 30 && abs(target.y_pixels) <= 30 && (target.target_type == HEAD || target.target_type == ISO_BALL || target.target_type == REYNA_FLASH))
//			{
//				target.priority += 30.f / abs(target.x_pixels) + 30.f / abs(target.y_pixels);
//			}
//		}
//
//		Target target;
//
//		for (auto& target_it : targets)
//		{
//			if (target_it.priority > target.priority)
//			{
//				target = target_it;
//			}
//		}
//
//		if (abs(target.x_pixels) <= 30 && abs(target.y_pixels) <= 30)
//		{
//			//config->addDebugMessage("Aimbot: " + std::to_string(target.x_pixels) + " " + std::to_string(target.y_pixels));
//		}
//		else {
//			continue;
//		}
//
//		float x_ratio = ratio_x(abs(target.x_pixels));
//		float y_ratio = ratio_y(abs(target.y_pixels));
//
//		float step = 1.0f / config->AIMBOT_TIME;
//
//		int x_aim_assist = roundf((target.x_movement * (aimbot_func(x_ratio) - aimbot_func(std::max(x_ratio - step, 0.f)))));
//		int y_aim_assist = roundf((target.y_movement * (aimbot_func(y_ratio) - aimbot_func(std::max(y_ratio - step, 0.f)))));
//
//		emulation->aimbot2(x_aim_assist, y_aim_assist);
//	}
//}

void Aimbot::loop() noexcept
{
	if (!config->AIMBOT_ON || config->AIMBOT_TIME == 0) return;
	if (target_detector->aimbot_new_target_found == false) return;

	std::vector<Target> targets;
	{
		std::lock_guard<std::mutex> lock(target_detector->targets_mutex);
		if (target_detector->targets == nullptr) return;
		if (target_detector->targets.get() == nullptr) return;
		targets = *target_detector->targets;
	}
	target = {};

	if (targets.size() == 0) return;

	const int aimbot_pixel_distance = target_detector->DegreesToPixelsX(config->AIMBOT_FOV / 2);

	for (auto& target_it : targets)
	{
		if (target_it.priority > this->target.priority)
		{
			this->target = target_it;
		}
	}

	int time_to_predict = config->NN_RUN_TIME.count() + config->EXTRAPOLATION_OFFSET + config->AIMBOT_TIME;
	auto time = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(time_to_predict);

	if (config->TOGGLE_EXTRAPOLATION_AIMBOT)
	{
		extrapolator->extrapolate(target, time);
		target_detector->UpdateTarget(target);
	}

	if (target.distance > aimbot_pixel_distance) return;
	if (target.distance <= 1.f) return;

	const float steps = config->AIMBOT_TIME;

	cached_x = target.x_movement;
	//cached_y = target.y_movement;
	is_in_motion = true;
	aimbot_points = bezier_engine->split_bezier_curve(steps);
	target_detector->aimbot_new_target_found = false;

	total_x = 0;
	//total_y = 0;

	for (auto const& point : aimbot_points)
	{
		int x_aimbot = std::roundf(cached_x * point.x);
		//int y_aimbot = std::roundf(cached_y * point.x);

		if (x_aimbot > 10000 || x_aimbot < -10000) return;
		//if (y_aimbot > 10000 || y_aimbot < -10000) return;

		if (std::abs(x_aimbot) < config->AIMBOT_DAMPENING) x_aimbot = 0;
		//if (std::abs(y_aimbot) < config->AIMBOT_DAMPENING) y_aimbot = 0;

		//target_detector->update_target_position(x_aimbot, y_aimbot);
		emulation->aimbot2(x_aimbot, 0);

		Sleep(1);

		total_x += x_aimbot;
		//total_y += y_aimbot;
	}

	is_in_motion = false;
}

void Aimbot::test_aimbot() noexcept
{
	std::vector<Target> targets;
	{
		std::lock_guard<std::mutex> lock(target_detector->targets_mutex);
		if (target_detector->targets == nullptr) return;
		if (target_detector->targets.get() == nullptr) return;
		targets = *target_detector->targets;
	}
	target = {};

	if (targets.size() == 0) return;

	const int aimbot_pixel_distance = target_detector->DegreesToPixelsX(config->AIMBOT_FOV / 2);

	for (auto& target : targets)
	{
		const auto distance = sqrt(pow(target.x_pixels, 2) + pow(target.y_pixels, 2));
		if (distance <= aimbot_pixel_distance && (target.target_type == HEAD || target.target_type == ISO_BALL || target.target_type == REYNA_FLASH))
		{
			target.priority += 30.f / abs(target.x_pixels) + 30.f / abs(target.y_pixels);
			target.distance = distance;
		}
	}

	for (auto& target_it : targets)
	{
		if (target_it.priority > this->target.priority)
		{
			this->target = target_it;
		}
	}

	if (target.distance > aimbot_pixel_distance) return;
	if (target.distance == 0.f) return;

	const float steps = config->AIMBOT_TIME;

	cached_x = target.x_movement;
	cached_y = target.y_movement;
	is_in_motion = true;
	aimbot_points = bezier_engine->split_bezier_curve(steps);

	int total_x = 0, total_y = 0;

	for (auto const& step : aimbot_points)
	{
		const int x_aimbot = std::roundf(cached_x * step.x);
		const int y_aimbot = std::roundf(cached_y * step.y);

		total_x += x_aimbot;
		total_y += y_aimbot;

		if (x_aimbot > 10000 || x_aimbot < -10000) return;
		if (y_aimbot > 10000 || y_aimbot < -10000) return;

		emulation->aimbot2(x_aimbot, y_aimbot);
		Sleep(1);
	}

	is_in_motion = false;
}
