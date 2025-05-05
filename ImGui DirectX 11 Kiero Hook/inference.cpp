#include "inference.h"

const std::vector<std::string> InferenceEngine::CLASS_NAMES = {
	"body", "head", "iso_ball", "reyna_flash" };

InferenceEngine::InferenceEngine() noexcept : session(nullptr)
{
}

InferenceEngine::~InferenceEngine() {}

void InferenceEngine::init(const std::string& model_path) {
	try {
		env = Ort::Env(ORT_LOGGING_LEVEL_VERBOSE, "InferenceEngine");

		session_options = Ort::SessionOptions();

		session_options.DisableMemPattern();
		session_options.SetIntraOpNumThreads(1);
		session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
		session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);

		OrtSessionOptionsAppendExecutionProvider_DML(session_options, 0);

		std::wstring widestr = std::wstring(model_path.begin(), model_path.end());
		const wchar_t* model_path_wchar = widestr.c_str();

		session = Ort::Session(env, model_path_wchar, session_options);

		// Check if the session was created successfully
		if (!session)
		{
			throw std::runtime_error("Failed to create ONNX Runtime session.");
		}
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

/*
 * Function to preprocess the image
 *
 * @param image: input image as cv::Mat
 * @return: vector of floats representing the preprocessed image
 */
std::vector<float> InferenceEngine::preprocessImage(const cv::Mat& image)
{
	if (image.empty())
	{
		throw std::runtime_error("Could not read the image");
	}

	cv::Mat resized_image;
	cv::resize(image, resized_image, cv::Size(gsl::at(input_shape, 2), gsl::at(input_shape, 3)));
	resized_image.convertTo(resized_image, CV_32F, 1.0 / 255);

	std::vector<cv::Mat> channels(3);
	cv::split(resized_image, channels);

	std::vector<float> input_tensor_values;
	input_tensor_values.reserve(gsl::at(input_shape, 1) * gsl::at(input_shape, 2) * gsl::at(input_shape, 3));

	for (int c = 0; c < 3; ++c)
	{
		input_tensor_values.insert(input_tensor_values.end(), (float*)channels[c].data, (float*)channels[c].data + gsl::at(input_shape, 2) * gsl::at(input_shape, 3));
	}

	return input_tensor_values;
}

/*
 * Function to filter the detections based on the confidence threshold
 *
 * @param results: vector of floats representing the output tensor
 * @param confidence_threshold: minimum confidence threshold
 * @param img_width: width of the input image
 * @param img_height: height of the input image
 * @param orig_width: original width of the image
 * @param orig_height: original height of the image
 * @return: vector of Detection objects
 */
std::vector<Detection> InferenceEngine::filterDetections(const std::vector<float>& results, int img_width, int img_height, int orig_width, int orig_height)
{
	std::vector<Detection> detections;
	const int num_detections = results.size() / 6;

	detections.reserve(num_detections);

	const float body_confidence_threshold = config->modelBodyConfidenceThreshold;
	const float head_confidence_threshold = config->modelHeadConfidenceThreshold;
	const float iso_ball_confidence_threshold = config->modelIsoBallConfidenceThreshold;
	const float reyna_flash_confidence_threshold = config->modelReynaFlashConfidenceThreshold;

	for (int i = 0; i < num_detections; ++i)
	{
		const float left = gsl::at(results, static_cast<gsl::index>(i) * 6 + 0);
		const float top = gsl::at(results, static_cast<gsl::index>(i) * 6 + 1);
		const float right = gsl::at(results, static_cast<gsl::index>(i) * 6 + 2);
		const float bottom = gsl::at(results, static_cast<gsl::index>(i) * 6 + 3);
		const float confidence = gsl::at(results, static_cast<gsl::index>(i) * 6 + 4);
		const int class_id = static_cast<int>(gsl::at(results, static_cast<gsl::index>(i) * 6 + 5));

		if (class_id == 0)
		{
			if (confidence >= body_confidence_threshold)
			{
				const int x = static_cast<int>(left * orig_width / img_width);
				const int y = static_cast<int>(top * orig_height / img_height);
				const int width = static_cast<int>((right - left) * orig_width / img_width);
				const int height = static_cast<int>((bottom - top) * orig_height / img_height);

				detections.push_back(
					{ confidence,cv::Rect(x, y, width, height),class_id,gsl::at(CLASS_NAMES, class_id) });
			}
		}

		if (class_id == 1)
		{
			if (confidence >= head_confidence_threshold)
			{
				const int x = static_cast<int>(left * orig_width / img_width);
				const int y = static_cast<int>(top * orig_height / img_height);
				const int width = static_cast<int>((right - left) * orig_width / img_width);
				const int height = static_cast<int>((bottom - top) * orig_height / img_height);

				detections.push_back(
					{ confidence,cv::Rect(x, y, width, height),class_id, gsl::at(CLASS_NAMES, class_id)});
			}
		}

		if (class_id == 2)
		{
			if (confidence >= iso_ball_confidence_threshold)
			{
				const int x = static_cast<int>(left * orig_width / img_width);
				const int y = static_cast<int>(top * orig_height / img_height);
				const int width = static_cast<int>((right - left) * orig_width / img_width);
				const int height = static_cast<int>((bottom - top) * orig_height / img_height);

				detections.push_back(
					{ confidence,cv::Rect(x, y, width, height),class_id,gsl::at(CLASS_NAMES, class_id) });
			}
		}

		if (class_id == 3)
		{
			if (confidence >= reyna_flash_confidence_threshold)
			{
				const int x = static_cast<int>(left * orig_width / img_width);
				const int y = static_cast<int>(top * orig_height / img_height);
				const int width = static_cast<int>((right - left) * orig_width / img_width);
				const int height = static_cast<int>((bottom - top) * orig_height / img_height);

				detections.push_back(
					{ confidence,cv::Rect(x, y, width, height),class_id,gsl::at(CLASS_NAMES, class_id) });
			}
		}
	}

	return detections;
}

/*
 * Function to run inference
 *
 * @param input_tensor_values: vector of floats representing the input tensor
 * @return: vector of floats representing the output tensor
 */
std::vector<float> InferenceEngine::runInference(const std::vector<float>& input_tensor_values)
{
	const Ort::AllocatorWithDefaultOptions allocator;

	std::string input_name = getInputName();
	std::string output_name = getOutputName();

	const char* input_name_ptr = input_name.c_str();
	const char* output_name_ptr = output_name.c_str();

	Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
	Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, const_cast<float*>(input_tensor_values.data()), input_tensor_values.size(), input_shape.data(), input_shape.size());

	auto output_tensors = session.Run(Ort::RunOptions{ nullptr }, &input_name_ptr, &input_tensor, 1, &output_name_ptr, 1);

	float* floatarr = gsl::at(output_tensors, 0).GetTensorMutableData<float>();
	size_t output_tensor_size = gsl::at(output_tensors, 0).GetTensorTypeAndShapeInfo().GetElementCount();

	return std::vector<float>(floatarr, floatarr + output_tensor_size);
}

/*
 * Function to get the input name
 *
 * @return: name of the input tensor
 */
std::string InferenceEngine::getInputName()
{
	Ort::AllocatorWithDefaultOptions allocator;
	Ort::AllocatedStringPtr name_allocator = session.GetInputNameAllocated(0, allocator);
	return std::string(name_allocator.get());
}

/*
 * Function to get the output name
 *
 * @return: name of the output tensor
 */
std::string InferenceEngine::getOutputName()
{
	Ort::AllocatorWithDefaultOptions allocator;
	Ort::AllocatedStringPtr name_allocator = session.GetOutputNameAllocated(0, allocator);
	return std::string(name_allocator.get());
}