#include "extrapolator.h"

void Extrapolator::init()
{
}

void Extrapolator::update(std::vector<Target> new_tracks)
{
	PastTargets targets = PastTargets{ new_tracks, std::chrono::high_resolution_clock::now() };
	
	{
		std::lock_guard<std::mutex> lock(past_targets_mutex);
		this->past_targets.push_back(std::make_shared<PastTargets>(targets));
		if (this->past_targets.size() > config->EXTRAPOLATION_BUFFER)
		{
			this->past_targets.pop_front();
		}
	}
}

void Extrapolator::extrapolate(Target& target, std::chrono::time_point<std::chrono::high_resolution_clock> time)
{
	int target_id = target.target_id;
	bool found_target = false;

	if (past_targets.empty())
	{
		return;
	}

	Kalman kalman_filter;
	bool initialized = false;

	for (const auto& past : past_targets)
	{
		auto time_point = past->time;
		std::chrono::duration<double> time_since_epoch = time_point.time_since_epoch();
		double t = time_since_epoch.count();

		auto it = std::find_if(past->targets.begin(), past->targets.end(),
			[target_id](const Target& t) { return t.target_id == target_id; });

		if (it != past->targets.end()) {
			const Target& past_target = *it;
			Eigen::Vector2d z;
			z << static_cast<double>(past_target.x_pixels), static_cast<double>(past_target.y_pixels);

			if (!initialized) {
				Eigen::Vector4d x0;
				x0 << z(0), z(1), 0.0, 0.0;
				kalman_filter.init(t, x0);
				initialized = true;
				found_target = true;
			}
			else {
				kalman_filter.predict(t);
				kalman_filter.update(z);
			}
		}
	}

	if (!found_target) {
		return;
	}

	std::chrono::duration<double> prediction_time_since_epoch = time.time_since_epoch();
	double t_prediction = prediction_time_since_epoch.count();

	kalman_filter.predict(t_prediction);
	Eigen::Vector4d x_hat = kalman_filter.state();

	this->extrapolated_x = static_cast<int>(std::round(x_hat(0)));
	this->extrapolated_y = static_cast<int>(std::round(x_hat(1)));

	this->movement_x = std::abs(target.x_pixels) - std::abs(this->extrapolated_x);
	this->movement_y = std::abs(target.y_pixels) - std::abs(this->extrapolated_y);

	target.x_pixels = static_cast<int>(std::round(x_hat(0)));
	target.y_pixels = static_cast<int>(std::round(x_hat(1)));
}

Kalman::Kalman()
    : x_hat(Eigen::Vector4d::Zero()), P(Eigen::Matrix4d::Identity()), F(Eigen::Matrix4d::Identity()),
    B(Eigen::Matrix<double, 4, 2>::Zero()), H(Eigen::Matrix<double, 2, 4>::Zero()),
    Q(Eigen::Matrix4d::Zero()), R(Eigen::Matrix2d::Identity()), I(Eigen::Matrix4d::Identity())
{
    H(0, 0) = 1.0;
    H(1, 1) = 1.0;
    R *= 1e-2;

    Q.block<2, 2>(0, 0) = Eigen::Matrix2d::Identity() * 1e-3;
    Q.block<2, 2>(2, 2) = Eigen::Matrix2d::Identity() * 1e-3;
}

void Kalman::init(double t0, const Eigen::Vector4d& x0)
{
	x_hat = x0;
	t_prev = t0;
}

void Kalman::computeF(double dt)
{
	F = Eigen::Matrix4d::Identity();
	F(0, 2) = dt;
	F(1, 3) = dt;
}

void Kalman::predict(double t)
{
	double dt = t - t_prev;
	computeF(dt);

	x_hat = F * x_hat;
	P = F * P * F.transpose() + Q;

	t_prev = t;
}

void Kalman::update(const Eigen::Vector2d& z)
{
	Eigen::Vector2d y = z - H * x_hat;
	Eigen::Matrix2d S = H * P * H.transpose() + R;
	Eigen::Matrix<double, 4, 2> K = P * H.transpose() * S.inverse();

	x_hat = x_hat + K * y;
	P = (I - K * H) * P;
}