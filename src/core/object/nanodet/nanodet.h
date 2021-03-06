#pragma once

#ifndef NANODET_H
#define NANODET_H

#include "infer/nanodet_backend.hpp"
#include "core/base/detection.h"

using namespace ObjectDetection;

class NanoDet : public Detection {
	typedef struct HeadInfo
	{
		std::string cls_layer;
		std::string dis_layer;
		int stride;
	};
public:
	NanoDet(const string &config_file);
	~NanoDet();
	void postProcessCPU(const shared_ptr<TRTInfer::Tensor>& cls_tensor, const shared_ptr<TRTInfer::Tensor>& loc_tensor,
		int stride, Size netInputSize, float threshold, int num_classes, vector<ccutil::BBox> &bboxs, int reg_max_);
	int EngineInference(const Mat& image, vector<ccutil::BBox>* result);
	int EngineInferenceOptim(const vector<Mat>& images, vector<vector<ccutil::BBox>>* result);

private:
	vector<int> strides_;
	vector<HeadInfo> heads_info_;
	vector<int> num_anchors_;
	map<int, string> detect_labels_;
	int reg_max_ = 7;
};


#endif // !NANODET_H


