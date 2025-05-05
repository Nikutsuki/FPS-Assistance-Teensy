#pragma once
#include "includes.h"
#include "Globals.h"

class Extrapolator : public virtual ExtrapolatorInstance
{
	void init() override;
	void update(std::vector<Target> new_tracks) override;
    void extrapolate(Target& target, std::chrono::time_point<std::chrono::high_resolution_clock> time) override;
};

class Kalman
{
public:
    Kalman();

    void init(double t0, const Eigen::Vector4d& x0);
    void predict(double t);
    void update(const Eigen::Vector2d& z);

    Eigen::Vector4d state() const { return x_hat; }
private:
    double t_prev;

    Eigen::Vector4d x_hat;
    Eigen::Matrix4d P;
    Eigen::Matrix4d F;
    Eigen::Matrix<double, 4, 2> B;
    Eigen::Matrix<double, 2, 4> H;
    Eigen::Matrix4d Q;
    Eigen::Matrix2d R;
    Eigen::Matrix4d I;

    void computeF(double dt);
};