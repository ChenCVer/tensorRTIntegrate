
#include "detection.h"

namespace ObjectDetection {

	Detection::Detection(){}
	Detection::~Detection(){}

	void Detection::LoadEngine() {

		INFO("LoadEngine...");
		if (!ccutil::exists(engine_file_)) {
			INFO("onnx to trtmodel...");
			if (!ccutil::exists(onnx_file_)) {
				INFOW("onnx file:%s not found !", onnx_file_.c_str());
				return;
			}
			TRTBuilder::compileTRT(
				TRTBuilder::TRTMode_FP16, {}, maxBatchSize_,
				TRTBuilder::ModelSource(onnx_file_), engine_file_,
				{ input_minDim_, input_optDim_, input_maxDim_ }
			);
		}
		INFO("load model: %s", engine_file_.c_str());
		engine_ = TRTInfer::loadEngine(engine_file_);
	}

	void Detection::preprocessImageToTensor(const Mat& image, int numIndex, const shared_ptr<TRTInfer::Tensor>& tensor) {

		int outH = tensor->height();
		int outW = tensor->width();
		float sw = outW / (float)image.cols;
		float sh = outH / (float)image.rows;
		float scale = std::min(sw, sh);
		cv::Mat flt_img = cv::Mat::zeros(cv::Size(outW, outH), CV_8UC3);
		cv::Mat outimage;
		cv::resize(image, outimage, cv::Size(), scale, scale);
		outimage.copyTo(flt_img(cv::Rect(0, 0, outimage.cols, outimage.rows)));
		float mean[3], std[3];
		for (int i = 0; i < 3; i++)
		{
			mean[i] = mean_[i];
			std[i] = std_[i];
		}
		tensor->setNormMatGPU(numIndex, flt_img, mean, std);
	}

	void Detection::outPutBox(vector<ccutil::BBox>& objs, const Size& imageSize, const Size& netInputSize, float minsize) {
		float sw = netInputSize.width / (float)imageSize.width;
		float sh = netInputSize.height / (float)imageSize.height;
		float scale = std::min(sw, sh);

		vector<ccutil::BBox> keep;
		for (int i = 0; i < objs.size(); ++i) {
			auto& obj = objs[i];
			obj.x = std::max(0.0f, std::min(obj.x / scale, imageSize.width - 1.0f));
			obj.y = std::max(0.0f, std::min(obj.y / scale, imageSize.height - 1.0f));
			obj.r = std::max(0.0f, std::min(obj.r / scale, imageSize.width - 1.0f));
			obj.b = std::max(0.0f, std::min(obj.b / scale, imageSize.height - 1.0f));

			if (obj.area() > minsize)
				keep.emplace_back(obj.x, obj.y, obj.r, obj.b, obj.score, obj.label);
		}
		objs = keep;
	}

}