
#include "face_alignment.h"

FaceAlignment::FaceAlignment() {}

FaceAlignment::FaceAlignment(const string &config_file) {

	YAML::Node root = YAML::LoadFile(config_file);
	YAML::Node config = root["face_alignment"];
	onnx_file_ = config["onnx_file"].as<std::string>();
	engine_file_ = config["engine_file"].as<std::string>();
	maxBatchSize_ = config["maxBatchSize"].as<int>();
	input_Dim_ = config["input_Dim"].as<std::vector<std::vector<int>>>();
	mean_ = config["mean"].as<std::vector<float>>();
	std_ = config["std"].as<std::vector<float>>();
	scale_ = config["scale"].as<float>();

	head_out_ = { "fc1" };
	LoadEngine();
}

FaceAlignment::~FaceAlignment() {}

void FaceAlignment::PrepareImage(const Mat& image, int numIndex, const shared_ptr<TRTInfer::Tensor>& tensor) {

	int outH = tensor->height();
	int outW = tensor->width();
	float sw = outW / (float)image.cols;
	float sh = outH / (float)image.rows;
	float scale_size = std::min(sw, sh);
	cv::Mat flt_img = cv::Mat::zeros(cv::Size(outW, outH), CV_8UC3);
	cv::Mat outimage;
	cv::resize(image, outimage, cv::Size(), scale_size, scale_size);
	outimage.copyTo(flt_img(cv::Rect(0, 0, outimage.cols, outimage.rows)));
	float mean[3], std[3];
	for (int i = 0; i < 3; i++)
	{
		mean[i] = mean_[i];
		std[i] = std_[i];
	}

	tensor->setNormMatGPU(numIndex, flt_img, mean, std, scale_);
}

int FaceAlignment::EngineInference(const Mat& image, vector<cv::Point2f>* result) {

	if (engine_ == nullptr) {
		INFO("EngineInference failure, model is nullptr");
		return -1;
	}
	vector<cv::Point2f>& alignment = *result;

	ccutil::Timer time_preprocess;
	engine_->input()->resize(1);
	Size imageSize = image.size();
	PrepareImage(image, 0, engine_->input());
	INFO("preprocess time cost = %f", time_preprocess.end());
	ccutil::Timer time_forward;
	engine_->forward();
	INFO("forward time cost = %f", time_forward.end());
	ccutil::Timer time_decode;
	
	auto out = engine_->tensor("fc1");
	auto outSize = out->channel();
	float* out_ptr = out->cpu<float>(0, 0);
	for (int index = 0; index < outSize; index += 2)
	{
		float x = (out_ptr[index] + 1) * imageSize.width / 2;
		float y = (out_ptr[index+1] + 1) * imageSize.height / 2;
		alignment.push_back(cv::Point2f(x, y));
	}
	INFO("decode time cost = %f", time_decode.end());

	return 0;
}

int FaceAlignment::EngineInferenceOptim(const vector<Mat>& images, vector<vector<cv::Point2f>>* result) {

	if (engine_ == nullptr) {
		INFO("EngineInference failure, model is nullptr");
		return -1;
	}
	vector<vector<cv::Point2f>>& alignments = *result;
	alignments.resize(images.size());

	ccutil::Timer time_preprocess;
	engine_->input()->resize(images.size());
	vector<Size> imagesSize;
	for (int i = 0; i < images.size(); i++) {
		PrepareImage(images[i], i, engine_->input());
		imagesSize.emplace_back(images[i].size());
	}
	INFO("preprocess time cost = %f", time_preprocess.end());
	ccutil::Timer time_forward;
	engine_->forward();
	INFO("forward time cost = %f", time_forward.end());
	ccutil::Timer time_decode;

	auto out = engine_->tensor("fc1");
	auto outSize = out->channel();
	for (int i = 0; i < images.size(); ++i) {
		Size imageSize = images[i].size();
		float* out_ptr = out->cpu<float>(i, 0);
		auto& alignment = alignments[i];
		for (int index = 0; index < outSize; index += 2)
		{
			float x = (out_ptr[index] + 1) * imageSize.width / 2;
			float y = (out_ptr[index + 1] + 1) * imageSize.height / 2;
			alignment.push_back(cv::Point2f(x, y));
		}
	}
	INFO("decode time cost = %f", time_decode.end());

	return 0;
}
