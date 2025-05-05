#include "aim_assist.h"

static float ratio_x(float x) noexcept
{
	return x / (static_cast<float>(NEURAL_NETWORK_INPUT_X) / 2);
}

static float ratio_y(float y) noexcept
{
	return y / (static_cast<float>(NEURAL_NETWORK_INPUT_Y) / 2);
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

void AimAssist::loop(int& x, int& y) noexcept
{
	if (!config->AIM_ASSIST_ON || config->AIM_ASSIST_TIME == 0) return;
	target = {};

	std::vector<Target> targets;
	{
		std::lock_guard<std::mutex> lock(target_detector->targets_mutex);
		targets = *target_detector->targets;
	}

	if (targets.size() == 0) return;

	select_func();

	const int aimbot_pixel_distance = target_detector->DegreesToPixelsX(config->AIM_ASSIST_FOV / 2);

	for (auto& target_it : targets)
	{
		if (target_it.priority > target.priority)
		{
			target = target_it;
		}
	}

	int time_to_predict = config->NN_RUN_TIME.count() + config->EXTRAPOLATION_OFFSET;
	auto time = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(time_to_predict);

	if (config->TOGGLE_EXTRAPOLATION_AIMASSIST)
	{
		extrapolator->extrapolate(target, time);
		target_detector->UpdateTarget(target);
	}

	if (target.distance >= aimbot_pixel_distance) return;

	const float step = 1.0f / config->AIM_ASSIST_TIME;

	//aim_correction(x, y, target);

	const float x_ratio = ratio_x(static_cast<float>(abs(target.x_pixels)));
	const float y_ratio = ratio_y(static_cast<float>(abs(target.y_pixels)));

	int x_aim_assist = static_cast<int>(roundf( (target.x_movement * (aimbot_func(x_ratio) - aimbot_func(std::max(x_ratio - step, 0.f))))));
	int y_aim_assist = static_cast<int>(roundf( (target.y_movement * (aimbot_func(y_ratio) - aimbot_func(std::max(y_ratio - step, 0.f))))));

	if (x_aim_assist > 10000 || x_aim_assist < -10000) return;
	if (y_aim_assist > 10000 || y_aim_assist < -10000) return;

	if (std::abs(x_aim_assist) < config->AIM_ASSIST_DAMPENING) x_aim_assist = 0;
	if (std::abs(y_aim_assist) < config->AIM_ASSIST_DAMPENING) y_aim_assist = 0;

	x += x_aim_assist;
	y += y_aim_assist;

	//target_detector->update_target_position(x, y);

	this->x_movement = x_aim_assist;
	this->y_movement = y_aim_assist;
}

void AimAssist::select_func() noexcept
{
	switch (config->aim_assist_mode)
	{
		case 0:
			aimbot_func = linear;
			break;
		case 1:
			aimbot_func = log_func;
			break;
		case 2:
			aimbot_func = sqrt_func;
			break;
		case 3:
			aimbot_func = sigmoid;
			break;
		case 4:
			aimbot_func = x_over_x;
			break;
		default:
			aimbot_func = sqrt_func;
			break;
	}
}