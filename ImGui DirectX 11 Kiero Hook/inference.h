#pragma once

#include "includes.h"
#include "Globals.h"

class InferenceEngine
{
public:
    InferenceEngine() noexcept;
    ~InferenceEngine();

    void init(const std::string& model_path);
    std::vector<float> preprocessImage(const cv::Mat& image);
    std::vector<Detection> filterDetections(const std::vector<float>& results, int img_width, int img_height, int orig_width, int orig_height);
    std::vector<float> runInference(const std::vector<float>& input_tensor_values);

    const std::vector<int64_t> input_shape = { 1, 3, NEURAL_NETWORK_INPUT_X, NEURAL_NETWORK_INPUT_Y };

private:
    Ort::Env env;
    Ort::SessionOptions session_options;
    Ort::Session session;

    std::string getInputName();
    std::string getOutputName();

    static const std::vector<std::string> CLASS_NAMES;
};